#include <ssl-tunnel/backend/tunnel_send.h>

#include <ssl-tunnel/lib/err.h>

#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>

#include <stdio.h>
#include <errno.h>
#include <unistd.h> // write
#include <arpa/inet.h> // ntohl

static void lookup_route(const proto_ip_t *packet, size_t packet_len, const trie_t *t,
                         peer_t **out_peer, bool *out_ok) {
    if (packet_len < PROTO_HEADER_IP_MIN_LEN) {
        // invalid packet;
        *out_ok = false;
        return;
    }

    trie_match(t, ntohl(packet->dst), out_peer, out_ok);
}

static void encrypt(const peer_t *p, const uint8_t *plaintext, int plaintext_len, uint8_t *key,
                    uint8_t *iv, size_t ciphertext_1) {
    if (!p->cfg_entry->cipher.present || p->cfg_entry->cipher.v == PROTO_CIPHER_NULL_SHA384) {
        return;
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (p->cfg_entry->cipher.v == PROTO_CIPHER_AES_256_GCM) {
        if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, key, iv)) {
            goto cleanup;
        }

        int len;
        uint8_t ciphertext[128];
        for (int i = 0; i < ciphertext_1; i += plaintext_len) {
            if (1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len)) {
                goto cleanup;
            }
        }

        if (1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) {
            goto cleanup;
        }
    } else if (p->cfg_entry->cipher.v == PROTO_CIPHER_CHACHA20_POLY1305) {
        if (1 != EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, key, iv)) {
            goto cleanup;
        }

        int len;
        uint8_t ciphertext[128];
        for (int i = 0; i < ciphertext_1; i += plaintext_len) {
            if (1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len)) {
                goto cleanup;
            }
        }

        if (1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) {
            goto cleanup;
        }
    } else {
        panicf("unknown cipher: %d", p->cfg_entry->cipher);
    }

cleanup:
    EVP_CIPHER_CTX_free(ctx);
}

static void verify(const peer_t *p, const uint8_t *key, size_t key_len, const uint8_t *data,
                   size_t data_len, uint8_t *digest) {
    if (!p->cfg_entry->cipher.present || p->cfg_entry->cipher.v != PROTO_CIPHER_NULL_SHA384) {
        return;
    }

    unsigned char hmac[EVP_MAX_MD_SIZE];
    unsigned int hmac_len = 0;

    HMAC(EVP_sha384(), key, key_len, (const unsigned char *)data, data_len, hmac, &hmac_len);
}

static void placeholder(const peer_t *p, size_t packet_len) {
    const size_t key_len = 32;

    uint8_t key[] = {
            0x04, 0x55, 0xad, 0x29, 0xee, 0xeb, 0xbc, 0x36,
            0xdd, 0x88, 0x7a, 0x59, 0xcb, 0x14, 0x87, 0x61,
            0xac, 0x9b, 0x54, 0x41, 0x50, 0x21, 0x5a, 0xad,
            0x3c, 0x61, 0x4b, 0xf0, 0xcb, 0x60, 0x68, 0x3f
    };
    uint8_t iv[] = {
            0x85, 0x79, 0xe3, 0xe9, 0xb4, 0x26, 0x18, 0x70,
            0x43, 0x70, 0xd6, 0x31
    };

    const char *data_enc = "Test data for AES-GCM";
    size_t data_len_enc = strlen(data_enc);

    encrypt(p, data_enc, data_len_enc, key, iv, packet_len);

    const char *data = "Test data for HMAC-SHA384";
    size_t data_len = strlen(data);
    verify(p, key, key_len, data, data_len, NULL);
}

void io_tun_read(int tun_fd, outbound_t *out, const trie_t *route_lookup) {
    // tun -> send_q
    int len = 0;

    while (len < OUTBOUND_BUF_SIZE) {
        outbound_packet_t *p = &out->out_q[len];

        ssize_t n = read(tun_fd, &p->packet, PROTO_MAX_MTU);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            panicf("error calling read (errno %d)", errno);
        }
        p->packet_len = n;

        bool ok;
        lookup_route(&p->packet, p->packet_len, route_lookup, &p->peer, &ok);
        if (!ok) {
            // route hasn't been matched; skip
            continue;
        }

        placeholder(p->peer, p->packet_len);

        len++;
    }

    if (len == 0) {
        return;
    }

    if (len == OUTBOUND_BUF_SIZE) {
        printf("tun_full\n");
        fflush(stdout);
    }

    pthread_mutex_lock(out->mx);
    for (int i = 0; i < len; i++) {
        deque_push_back((deque_any_t *) out->send_q, &out->out_q[i]);
    }
    pthread_mutex_unlock(out->mx);
}

void io_udp_write(int socket_fd, outbound_t *out, uint32_t self_index) {
    // send_q -> udp
    int len = 0;

    pthread_mutex_lock(out->mx);
    while (len < OUTBOUND_BUF_SIZE) {
        if (out->send_q->len == 0) {
            break;
        }

        err_t err;
        if (!ERROR_OK(err = deque_front((deque_any_t *) out->send_q, &out->out_q[len]))) {
            panicf("unable to pop front: %s", err.msg);
        }

        if (!ERROR_OK(err = deque_pop_front((deque_any_t *) out->send_q))) {
            panicf("error popping front of send_q: %s", err.msg);
        }

        if (!out->out_q[len].peer->is_remote_addr_known) {
            continue;
        }

        len++;
    }
    pthread_mutex_unlock(out->mx);

    for (int i = 0; i < len; i++) {
        outbound_packet_t *p = &out->out_q[i];

        size_t write_len;
        proto_transport_t packet;
        err_t err = proto_new_transport_packet(self_index, (uint8_t[12]) {0}, (const uint8_t *) &p->packet, p->packet_len, &packet, &write_len);
        if (!ERROR_OK(err)) {
            printf("error creating new packet: %s (%lu)", err.msg, p->packet_len);
            continue;
        }

        ssize_t n = sendto(socket_fd, &packet, write_len, 0, (struct sockaddr *) &p->peer->remote_addr,
                           p->peer->remote_addr_len);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                printf("warn: udp dropping packets!!!\n");
                fflush(stdout);
                break;
            }

            panicf("error calling sendto (errno %d)", errno);
        }
        if ((size_t) n != write_len) {
            panicf("bytes sent and total buffered bytes did not match");
        }
    }
}
