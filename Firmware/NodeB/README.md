# Node B – BLE Scanner

Node B er en BLE-scanner basert på nRF53 (Thingy:91 X) som mottar
BLE advertising-pakker fra Node A-enheter og parser
**Manufacturer Specific Data**.  
All mottatt informasjon logges via RTT.

Applikasjonen er utviklet for å fungere med både en simulert ESP32-basert
Node A og en planlagt Node A-enhet med utvidet payload-format.

---

## BLE Manufacturer Data Format

Node B scanner kontinuerlig etter BLE advertising-pakker og analyserer
Manufacturer Specific Data-feltet for å hente ut sensordata.

---

### Legacy format (ESP32 Node A – midlertidig)

Dette formatet brukes av den nåværende ESP32-baserte Node A-simuleringen.

| Offset | Størrelse | Felt                       |
|------:|----------:|----------------------------|
| 0     | 2 bytes   | Company ID (Little Endian) |
| 2     | 1 byte    | Node ID                    |
| 3     | 1 byte    | Sequence number            |
| 4     | 2 bytes   | Temperature (°C ×100)      |

```c
struct legacy_payload {
    uint16_t company_id;
    uint8_t  node_id;
    uint8_t  seq;
    int16_t  temperature;
};
``` 

### Planned / Final format (adv_mfg_data_type)
Dette formatet er valgt som felles prosjekt-protokoll og brukes av
partner Node A-enheten.

| Offset | Størrelse | Felt         | Beskrivelse             |
| -----: | --------: | ------------ | ----------------------- |
|      0 |   2 bytes | company_code | Company Identifier Code |
|      2 |    1 byte | magic        | Protocol identifier     |
|      3 |    1 byte | ver          | Protocol version        |
|      4 |   2 bytes | seq          | Sequence number         |
|      6 |   2 bytes | t_c_e2       | Temperature (°C ×100)   |
|      8 |   2 bytes | rh_e2        | Humidity (% ×100)       |
|     10 |   4 bytes | lat_e7       | Latitude ×1e7           |
|     14 |   4 bytes | lon_e7       | Longitude ×1e7          |
|     18 |    1 byte | src          | Source / team number    |


```c
typedef struct __packed adv_mfg_data {
    uint16_t company_code;
    uint8_t  magic;
    uint8_t  ver;
    uint16_t seq;
    int16_t  t_c_e2;
    uint16_t rh_e2;
    int32_t  lat_e7;
    int32_t  lon_e7;
    uint8_t  src;
} adv_mfg_data_type;

``` 