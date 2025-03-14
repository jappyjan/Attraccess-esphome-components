// API communication methods
bool send_api_request_(esphome::attraccess_desfire::ApiCommandType cmd_type, const std::vector<uint8_t> &data);
bool handle_api_response_();
bool send_button_event_();
bool send_card_detected_(const std::string &uid);
bool execute_card_operation_(esphome::attraccess_desfire::CardOpType op_type, const std::vector<uint8_t> &data);
void handle_authentication_result_(bool success);
std::vector<uint8_t> build_binary_packet_(esphome::attraccess_desfire::ApiCommandType cmd_type, uint16_t cmd_id, const std::vector<uint8_t> &data);
bool parse_binary_packet_(const std::vector<uint8_t> &packet, esphome::attraccess_desfire::ApiCommand &command);
uint16_t get_next_cmd_id_();