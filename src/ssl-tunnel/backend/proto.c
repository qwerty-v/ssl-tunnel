#include <ssl-tunnel/backend/proto.h>

#include <ssl-tunnel/lib/err.h>

const err_t ERR_DATA_TOO_LARGE = {
    .msg = "data is too large"
};

err_t proto_new_transport_packet(uint32_t remote_index, uint64_t nonce, const uint8_t *data, size_t data_len,
                                 proto_transport_t *out_packet, size_t *out_len) {
    memset(out_packet, 0, sizeof(proto_transport_t));

    out_packet->packet_type = PROTO_PACKET_TYPE_TRANSPORT;
    out_packet->remote_index = remote_index;
    out_packet->nonce = nonce;

    if (data_len > PROTO_TRANSPORT_MAX_DATA_LEN) {
        return ERR_DATA_TOO_LARGE;
    }

    memcpy(out_packet->data, data, data_len);
    *out_len = PROTO_HEADER_TRANSPORT_LEN + data_len;

    return ENULL;
}
