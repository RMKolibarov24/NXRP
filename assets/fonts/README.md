# Interface font

The application uses **Source Sans 3 Semibold**, an Adobe font designed for user
interfaces. The unmodified font and full SIL Open Font License are bundled here.

- Source: https://github.com/adobe-fonts/source-sans/tree/release/TTF
- File: `SourceSans3-Semibold.ttf`
- SHA-256: `A3F4F8DCF343A8F24DC61951DE93F3BA1558B15CD250BA24AF8A40E957081B7D`
- License: `SourceSans3-LICENSE.md`

Other candidates are [Inter](https://rsms.me/inter/) and
[Noto Sans](https://github.com/notofonts/latin-greek-cyrillic).

The SFML text is positioned on whole pixels, font texture smoothing is disabled,
and resizing updates the view to keep one view unit equal to one window pixel.
These measures avoid additional blur from fractional positioning, filtering and
stretching. The operating system's display scaling can still affect appearance.
