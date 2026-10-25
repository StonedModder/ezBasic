# Third-party notices

## Plutonium

The bundled UI framework is [Plutonium](https://github.com/XorTroll/Plutonium) by XorTroll, pinned locally at commit `c98c7e1837c76824b27284709c48429d1407e390`. Its MIT license is included as [`Plutonium/LICENSE`](Plutonium/LICENSE).

## Plume USB

The USB device enumeration and HID readiness work was informed by [Plume USB](https://github.com/MichelSama/plume-usb) by MichelSama and the Plume USB contributors. The upstream project is MIT-licensed; the license notice is reproduced below.

```text
MIT License

Copyright (c) 2026 Plume USB contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

## libnx

The build uses [libnx](https://github.com/switchbrew/libnx), the Nintendo Switch homebrew library, under its ISC license. The library is a devkitPro dependency and is not vendored in this repository. Its license notice is reproduced here for the distributed NRO:

```text
Copyright 2017-2018 libnx Authors

Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted, provided that the above copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
```

## Embedded SmileBASIC bootstrap

`romfs/bootstrap.bas` is from [kurumuz/ns2jb](https://gitlab.com/kurumuz/ns2jb), a fork of [pepsipu/ns2jb](https://gitlab.com/pepsipu/ns2jb). The payload is embedded in the source tree and in the built NRO. The public upstream projects' default-branch file listings contain no `LICENSE` file, and GitLab reports no recognized project license. This repository does not claim ownership of the bootstrap or grant rights to redistribute it. Obtain permission from the relevant upstream rights holder before publishing the source or an NRO containing this payload.

## ezBasic

The original ezBasic code and assets are attributed to StonedModder. No separate license grant for those materials is made by this notice.
