:floppy_disk: EEPROM utilities for Arduino Portenta X8
======================================================
[![Smoke test status](https://github.com/pika-spark/portenta-x8-eeprom-utils/actions/workflows/smoke-test.yml/badge.svg)](https://github.com/pika-spark/portenta-x8-eeprom-utils/actions/workflows/smoke-test.yml)
[![Spell Check status](https://github.com/pika-spark/portenta-x8-eeprom-utils/actions/workflows/spell-check.yml/badge.svg)](https://github.com/pika-spark/portenta-x8-eeprom-utils/actions/workflows/spell-check.yml)

This work is based on [`raspberrypi/hats`](https://github.com/raspberrypi/hats).

### How-to-build
```bash
mkdir build && cd build
cmake .. && make
sudo make install
```

### How-to-use
**PC**
* Edit `eeprom_setting.txt` to suit your specific HAT.
* Run `./eepmake eeprom_settings.txt eeprom.eep` to create the eep binary.

**Portenta X8**
* Use Docker and `eepflash` to flash the configuration to your EEPROM, i.e.
```bash
eepflash.sh --write --device=1 --address=50 -t=24c256 -f eeprom.eep
```
*Note*: Ensure that write protection - if you have any - is disabled.

### Examples
* [`eeprom-flash`](https://github.com/pika-spark/pika-spark-containers/tree/main/eeprom-flash) flashes the EEPROM of the [Pika Spark](https://pika-spark.io/).
* [`eeprom-dump`](https://github.com/pika-spark/pika-spark-containers/tree/main/eeprom-dump) dumps the content of the EEPROM of the [Pika Spark](https://pika-spark.io/) to `stdout`.
