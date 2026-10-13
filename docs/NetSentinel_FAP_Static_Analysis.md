# NetSentinel Ultra — static analysis of owner-supplied FAP references

Analysis is static only. No FAP executed, no hardware accessed. Imports and function symbols show code dependencies, not verified execution paths or Ultra V3 physical routing.

| Filename | Bytes | SHA-256 | External imports |
|---|---:|---|---:|
| `esp32_wifi_marauder.fap` | 109880 | `6e6c44e3f5d6f43260e6e492e92653cdc1c9f21bad9735dc5e6b52e656dde9f6` | 159 |
| `marauder_gui.fap` | 146216 | `01e7bb85118ccb5b6163b623d0078c7f377643925eeb708adb975050c2f16937` | 131 |
| `nrf24batch.fap` | 42980 | `26e9120cbb16f1001298cf2c2e37283ff86d5460b449124b92648e76af09ad68` | 100 |
| `nrf24channelscanner.fap` | 8660 | `5bce6cf551c38f6f704591ec76da3dff35e0c954e5dceb23b868e17e1e1c8e26` | 50 |
| `nrf24mousejacker.fap` | 22340 | `31669f87678bcec3428232143dc2f6e0e8deb373b96d0fce8747d52b7a2fd1e6` | 77 |
| `nrf24scan.fap` | 32796 | `3fa232a41207fe8b0b310b842a430cb73b618b83c0232d8239aeaa4896386e90` | 82 |
| `nrf24sniff.fap` | 16884 | `85139fd233b3ed265515ef9ed2486c0c87b62c7800be7c499735d719334b9062` | 60 |

## esp32_wifi_marauder.fap

### Relevant imported symbols

- `furi_hal_power_enable_otg`
- `momentum_settings`
- `furi_hal_serial_tx`
- `furi_hal_serial_control_release`
- `furi_hal_power_is_otg_enabled`
- `furi_hal_serial_control_acquire`
- `furi_hal_serial_async_rx_start`
- `furi_hal_serial_async_rx_stop`
- `furi_hal_serial_deinit`
- `furi_hal_power_disable_otg`
- `furi_hal_serial_async_rx`
- `furi_hal_serial_init`

### Relevant internal function symbols

- `uart_worker`
- `wifi_marauder_uart_set_handle_rx_data_cb`
- `wifi_marauder_uart_set_handle_rx_pcap_cb`
- `wifi_marauder_uart_on_irq_cb`
- `wifi_marauder_uart_tx`
- `wifi_marauder_uart_free`
- `wifi_marauder_uart_init`

## marauder_gui.fap

### Relevant imported symbols

- `furi_hal_power_enable_otg`
- `furi_hal_serial_tx`
- `furi_hal_serial_async_rx_available`
- `furi_hal_serial_control_release`
- `furi_hal_power_is_otg_enabled`
- `furi_hal_serial_control_acquire`
- `furi_hal_serial_async_rx_start`
- `furi_hal_serial_async_rx_stop`
- `furi_hal_serial_deinit`
- `furi_hal_power_disable_otg`
- `furi_hal_serial_async_rx`
- `furi_hal_serial_init`

### Relevant internal function symbols

- `marauder_uart_rx_callback`
- `marauder_uart_send_line`
- `marauder_uart_receive_pcap`
- `marauder_uart_alloc`
- `marauder_uart_free`
- `marauder_uart_reset_capture`
- `marauder_uart_send`
- `marauder_uart_receive`

## nrf24batch.fap

### Relevant imported symbols

- `furi_hal_power_enable_otg`
- `furi_hal_spi_bus_handle_external_extra`
- `momentum_settings`
- `furi_hal_gpio_init_simple`
- `furi_hal_spi_bus_handle_init`
- `furi_hal_power_is_otg_enabled`
- `furi_hal_spi_bus_handle_external`
- `furi_hal_spi_bus_handle_deinit`
- `gpio_ext_pc3`
- `furi_hal_spi_release`
- `furi_hal_spi_bus_trx`
- `furi_hal_spi_acquire`
- `furi_hal_power_get_battery_current`
- `furi_hal_power_disable_otg`
- `furi_hal_power_is_charging`
- `gpio_ext_pb2`
- `gpio_ext_pa4`
- `furi_hal_gpio_init`

### Relevant internal function symbols

- `prepare_nrf24`
- `nrf24_resend_read_packet`
- `nrf24_set_maclen`
- `nrf24_status`
- `nrf24_read_newpacket`
- `nrf24_send_packet`
- `nrf24_get_packetlen`
- `nrf24_txpacket`
- `nrf24_flush_rx`
- `nrf24_set_tx_mode`
- `nrf24_init`
- `nrf24_flush_tx`
- `nrf24_rxpacket`
- `nrf24_read_register`
- `nrf24_write_reg`
- `nrf24_spi_trx`
- `nrf24_read_reg`
- `nrf24_set_rx_mode`
- `nrf24_write_buf_reg`
- `nrf24_set_idle`
- `nrf24_set_mac`
- `nrf24_deinit`
- `nrf24batch_app`

## nrf24channelscanner.fap

### Relevant imported symbols

- `furi_hal_power_enable_otg`
- `furi_hal_gpio_init_simple`
- `furi_hal_spi_bus_handle_init`
- `furi_hal_power_is_otg_enabled`
- `furi_hal_spi_bus_handle_external`
- `furi_hal_spi_bus_handle_deinit`
- `gpio_ext_pc3`
- `furi_hal_spi_release`
- `furi_hal_spi_bus_trx`
- `furi_hal_spi_acquire`
- `furi_hal_power_disable_otg`
- `gpio_ext_pb2`
- `furi_hal_gpio_init`

### Relevant internal function symbols

- `nrf24_get_rdp`
- `nrf24_status`
- `nrf24_flush_rx`
- `nrf24_check_connected`
- `nrf24_init`
- `nrf24channelscanner_main`
- `nrf24_write_reg`
- `nrf24_spi_trx`
- `nrf24_read_reg`
- `nrf24_set_rx_mode`
- `nrf24_set_idle`
- `nrf24_deinit`

## nrf24mousejacker.fap

### Relevant imported symbols

- `furi_hal_power_enable_otg`
- `furi_hal_spi_bus_handle_external_extra`
- `momentum_settings`
- `furi_hal_gpio_init_simple`
- `furi_hal_spi_bus_handle_init`
- `furi_hal_power_is_otg_enabled`
- `furi_hal_spi_bus_handle_external`
- `furi_hal_spi_bus_handle_deinit`
- `gpio_ext_pc3`
- `furi_hal_spi_release`
- `furi_hal_spi_bus_trx`
- `furi_hal_spi_acquire`
- `furi_hal_power_disable_otg`
- `gpio_ext_pb2`
- `gpio_ext_pa4`
- `furi_hal_gpio_init`

### Relevant internal function symbols

- `nrf24_set_dst_mac`
- `nrf24_set_maclen`
- `nrf24_status`
- `nrf24_txpacket`
- `nrf24_configure`
- `nrf24_flush_rx`
- `nrf24_set_tx_mode`
- `nrf24_set_src_mac`
- `nrf24_check_connected`
- `nrf24_init`
- `nrf24_flush_tx`
- `nrf24_write_reg`
- `nrf24_spi_trx`
- `nrf24_read_reg`
- `nrf24_write_buf_reg`
- `nrf24_set_idle`
- `nrf24_deinit`
- `nrf24_find_channel`

## nrf24scan.fap

### Relevant imported symbols

- `furi_hal_power_enable_otg`
- `furi_hal_spi_bus_handle_external_extra`
- `momentum_settings`
- `furi_hal_gpio_init_simple`
- `furi_hal_spi_bus_handle_init`
- `furi_hal_power_is_otg_enabled`
- `furi_hal_spi_bus_handle_external`
- `furi_hal_spi_bus_handle_deinit`
- `gpio_ext_pc3`
- `furi_hal_spi_release`
- `furi_hal_spi_bus_trx`
- `furi_hal_spi_acquire`
- `furi_hal_power_disable_otg`
- `gpio_ext_pb2`
- `gpio_ext_pa4`
- `furi_hal_gpio_init`

### Relevant internal function symbols

- `prepare_nrf24`
- `nrf24_set_maclen`
- `nrf24_status`
- `nrf24_read_newpacket`
- `nrf24_send_packet`
- `nrf24_get_packetlen`
- `nrf24_txpacket`
- `nrf24_flush_rx`
- `nrf24_set_tx_mode`
- `nrf24scan_app`
- `nrf24_init`
- `nrf24_flush_tx`
- `nrf24_rxpacket`
- `nrf24_write_reg`
- `nrf24_spi_trx`
- `nrf24_read_reg`
- `nrf24_set_rx_mode`
- `nrf24_write_buf_reg`
- `nrf24_set_idle`
- `nrf24_set_mac`
- `nrf24_deinit`

## nrf24sniff.fap

### Relevant imported symbols

- `furi_hal_power_enable_otg`
- `furi_hal_spi_bus_handle_external_extra`
- `momentum_settings`
- `furi_hal_gpio_init_simple`
- `furi_hal_spi_bus_handle_init`
- `furi_hal_power_is_otg_enabled`
- `furi_hal_spi_bus_handle_external`
- `furi_hal_spi_bus_handle_deinit`
- `gpio_ext_pc3`
- `furi_hal_spi_release`
- `furi_hal_spi_bus_trx`
- `furi_hal_spi_acquire`
- `furi_hal_power_disable_otg`
- `gpio_ext_pb2`
- `gpio_ext_pa4`
- `furi_hal_gpio_init`

### Relevant internal function symbols

- `nrf24_set_dst_mac`
- `nrf24_set_maclen`
- `nrf24_status`
- `nrf24_get_packetlen`
- `nrf24_init_promisc_mode`
- `nrf24_txpacket`
- `nrf24_configure`
- `nrf24_flush_rx`
- `nrf24_sniff_address`
- `nrf24_set_tx_mode`
- `nrf24_set_src_mac`
- `nrf24_check_connected`
- `nrf24_init`
- `nrf24_flush_tx`
- `nrf24_rxpacket`
- `nrf24_write_reg`
- `nrf24_spi_trx`
- `nrf24_read_reg`
- `nrf24_write_buf_reg`
- `nrf24_set_idle`
- `nrf24_set_packetlen`
- `nrf24_deinit`
- `nrf24_find_channel`

## Conclusions

1. Five NRF24 applications use Flipper external SPI via `furi_hal_spi_acquire`, `furi_hal_spi_bus_trx`, and `furi_hal_spi_release`; the imported symbol does not demonstrate bounded acquisition.
2. `nrf24sniff.fap` imports both standard and extra external SPI handles, `momentum_settings`, and GPIOs PA4, PB2 and PC3. Which path is used with the Ultra V3 requires control-flow analysis and hardware confirmation.
3. `nrf24channelscanner.fap` is a simpler reference using the standard external SPI handle, PB2 and PC3.
4. Multiple NRF24 programs have transmit-capable routines (`nrf24_txpacket` and/or `nrf24_send_packet`); do not reuse transmit paths in the NetSentinel no-write diagnostic.
5. Both Marauder apps import the serial control/receive API; they can inform protocol and lifetime handling but neither proves passive-only ESP32 operation.
6. Physical pin routing, selector behavior, true SPI acquisition deadlines, actual NRF24 readout, and CC1101 provider linkage remain unresolved.

## Suggested next step

Use these exact imported symbol names and function symbols to locate the matching permitted SDK interfaces and publicly available source. First validate disassembly control flow of `nrf24_init`, `nrf24_spi_trx`, and `nrf24_deinit` without executing uploaded binaries. Implement a narrowly gated diagnostic test only after the routing and blocking acquisition questions are answered.
