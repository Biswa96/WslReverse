# WSL_Reverse
Reverse engineering WSL

Reveal hidden COM interface between WSL and Lxss Manager Service. Heavily inspired by kernel guru Alex Ionescu's project [lxss](https://github.com/ionescu007/lxss). 

## Usage

This project only shows the hidden COM methods which may change in future Windows version. Use Windows Insider builds or download ISO from https://uup.rg-adguard.net/. Check out the Others folder. e.g. [Lxss_Service.REG](Others/Lxss_Service.REG) file unleashes the hidden beast. 

```
Usage: WslReverse.exe [-] [option] [argument]
Options:
    -d, --get-id       [distribution name]      Get distribution ID.
    -G, --get-default                           Get default distribution ID.
    -g, --get-config   [distribution name]      Get distribution configuration.
    -h, --help                                  Show list of options.
    -i, --install      [distribution name]      Install distribution (run as administrator).
    -r, --run          [distribution name]      run a Linux binary (incomplete feature).
    -S, --set-default  [distribution name]      Set default distribution.
    -s, --set-config   [distribution name]      Set configuration for distribution.
    -u, --uninstall    [distribution name]      Uninstall distribution.
```

## License 

This project is licensed under GNU Public License v3 or higher.

```
WslReverse -- (c) Copyright 2018 Biswapriyo Nath

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
```
