#include <ssl-tunnel/backend/tunnel_recv.h>

#include <ssl-tunnel/lib/err.h>

#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>

#include <errno.h>
#include <unistd.h> // write
#include <stdio.h> // fprintf

static void lookup_index(const proto_transport_t *packet, size_t packet_len, const hashmap_t *h,
                         peer_t **out_peer, bool *out_ok) {
    if (packet_len < PROTO_HEADER_TRANSPORT_LEN) {
        // invalid packet;
        *out_ok = false;
        return;
    }

    hashmap_get(h, packet->remote_index, out_peer, out_ok);
}

static void decrypt(const peer_t *p, uint8_t *ciphertext_1, int ciphertext_len, uint8_t *key,
                    uint8_t *iv, uint8_t *plaintext_1) {
    if (!p->cfg_entry->cipher.present || p->cfg_entry->cipher.v == PROTO_CIPHER_NULL_SHA384) {
        return;
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (p->cfg_entry->cipher.v == PROTO_CIPHER_AES_256_GCM) {
        if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, key, iv)) {
            goto cleanup;
        }
        uint8_t ciphertext[21] = {
                0x1a, 0x9a, 0xde, 0xa0, 0x8c, 0xde, 0xf5, 0x34, 0xbc, 0x7d, 0xa4, 0xb2, 0x78, 0x26, 0xc9, 0x11, 0x51, 0x9b, 0x54, 0xbe, 0xce
        };

        uint8_t plaintext[64];
        int len;
        for (int i = 0; i < ciphertext_len; i += 21) {
            if (1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, 21)) {
                goto cleanup;
            }
        }
    } else if (p->cfg_entry->cipher.v == PROTO_CIPHER_CHACHA20_POLY1305) {
        if (1 != EVP_DecryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, key, iv)) {
            goto cleanup;
        }
        uint8_t ciphertext[21] = {
                0x6e, 0xa1, 0x0c, 0xc8, 0x7a, 0x31, 0xf6, 0x61, 0x31, 0x02, 0x18, 0x9c, 0xce, 0xa1, 0xc2, 0xc3, 0x58, 0xfd, 0xa2, 0x26, 0x76
        };

        uint8_t plaintext[64];
        int len;
        for (int i = 0; i < ciphertext_len; i += 21) {
            if (1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, 21)) {
                goto cleanup;
            }
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

    decrypt(p, NULL, packet_len, key, iv, NULL);

    const char *data = "Test data for HMAC-SHA384";
    size_t data_len = strlen(data);
    verify(p, key, key_len, data, data_len, NULL);
}

void io_udp_read(int socket_fd, inbound_t *in, const hashmap_t *index_lookup) {
    // socket -> recv_q
    struct sockaddr_in addrs[INBOUND_BUF_SIZE];
    int len = 0;

    while (len < INBOUND_BUF_SIZE) {
        inbound_packet_t *p = &in->in_q[len];

        socklen_t addr_len = sizeof(struct sockaddr_in);

        ssize_t n = recvfrom(socket_fd, &p->packet, PROTO_MAX_MTU, 0, (struct sockaddr *) &addrs[len], &addr_len);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            panicf("error calling recvfrom (errno %d)", errno);
        }
        p->packet_len = n;

        bool ok;
        lookup_index(&p->packet, p->packet_len, index_lookup, &p->peer, &ok);
        if (!ok) {
            // unknown peer; skip
            continue;
        }

        placeholder(p->peer, p->packet_len);

        len++;
    }

    if (len == 0) {
        return;
    }

    if (len == INBOUND_BUF_SIZE) {
        printf("udp_full\n");
        fflush(stdout);
    }

    pthread_mutex_lock(in->mx);
    for (int i = 0; i < len; i++) {
        inbound_packet_t *p = &in->in_q[i];

        if (!p->peer->is_remote_addr_known) {
            p->peer->remote_addr = addrs[i];
            p->peer->remote_addr_len = sizeof(struct sockaddr_in);

            p->peer->is_remote_addr_known = true;
        }

        deque_push_back((deque_any_t *) in->recv_q, p);
    }
    pthread_mutex_unlock(in->mx);
}

void io_tun_write(int tun_fd, inbound_t *in) {
    // recv_q -> tun
    int len = 0;

    pthread_mutex_lock(in->mx);
    for (int i = 0; i < INBOUND_BUF_SIZE; i++) {
        if (in->recv_q->len == 0) {
            break;
        }

        err_t err = deque_front((deque_any_t *) in->recv_q, &in->in_q[i]);
        if (!ERROR_OK(err)) {
            panicf("unable to pop front: %s", err.msg);
        }

        if (in->in_q[i].packet_len <= PROTO_HEADER_TRANSPORT_LEN) {
            panicf("packet len too small");
        }

        if (!ERROR_OK(err = deque_pop_front((deque_any_t *) in->recv_q))) {
            panicf("error removing queue element");
        }

        len++;
    }
    pthread_mutex_unlock(in->mx);

    for (int i = 0; i < len; i++) {
        size_t write_len = in->in_q[i].packet_len - PROTO_HEADER_TRANSPORT_LEN;

        ssize_t n = write(tun_fd, in->in_q[i].packet.data, write_len);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                printf("warn: tun dropping packets!!!\n");
                fflush(stdout);
                break;
            }

            panicf("error calling write (errno %d)", errno);
        }
        if ((size_t) n != write_len) {
            panicf("unable to write whole buffer into device");
        }
    }
}
