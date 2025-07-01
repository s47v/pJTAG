# pJTAG

This code is based on the [pico-dirtyJtag](https://github.com/phdussud/pico-dirtyJtag) project and implements the necessary JTAG password authentication flow and the glitch injection for the SPC58 and MPC5748G

| Pin name | GPIO   | Pico Pin Number |
|:---------|:-------| ----------------|
| TDI      | GPIO17 | 22         	  |
| TDO      | GPIO16 | 21         	  |
| TCK      | GPIO18 | 24              |	
| TMS      | GPIO19 | 25              |
| RST      | GPIO20 | 26              |
| TRST     | GPIO21 | 27              |
| PORST (MPC only)    | GPIO22 | 29		      |
| Glitch   | GPIO14 | 19 		      |



## Usage
To start glitch attempts use `run.py`, to send optional arguments and log output of Pico.

| Argument                |Description                                                                                                                                                                     |
| ----------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `-p`, `--port`          | Serial port where the Pico is connected. Defaults to `/dev/ttyACM0`                                                                                    |
| `-o`, `--Output`        | Logfile name for saving the Pico's output. Defaults to `pico_output.log`. Rotates after reaching 4GB                                                                                                       |
| `-l`, `--glitch_length` | Glitch length range in the format `MIN,MAX` (e.g., `20,70`). Defaults to `20,70` (MPC), `20,48` (SPC)                                                                                                                  |
| `-d`, `--glitch_delay`  | Glitch delay range in the format `MIN,MAX` (e.g., `100,2000`). Defaults to `0,2000` (MPC), `0,1500` (SPC)                                                                                                                  |
| `-x`, `--password`      | Path to a password file containing a 64-character hex string (representing 32 bytes, see `pw_mpc.txt`). Defaults to glitch password with 1 byte set to 0 |

### Example Usage:
```
python3 run.py -p /dev/ttyACM0 -o glitch_log.txt -l 30,50 -d 10,1800 -x password.txt
```
