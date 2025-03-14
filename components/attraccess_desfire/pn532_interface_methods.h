// PN532 communication methods
bool init_pn532_();
bool send_command_(uint8_t command, const uint8_t *data = nullptr, uint8_t data_len = 0);
bool read_response_(uint8_t command, uint8_t *response, uint8_t *response_len, uint16_t timeout = 1000);
bool wait_ready_(uint16_t timeout = 1000);
bool transceive_data_(const uint8_t *tx_data, uint8_t tx_len, uint8_t *rx_data, uint8_t *rx_len);