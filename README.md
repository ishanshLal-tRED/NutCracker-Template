
Helper command for generating font atlasses.

pushd assets
.\executables\msdf-atlas-gen.exe -font '.\fonts\Shonen Punk! Custom Bold Bold.ttf' -charset '..\fonts\charset.txt' -fontname 'Shonen punk!' -type mtsdf -format png -imageout ..\fonts_atlas\shonen_punk.png -json ..\fonts_atlas\shonen_punk.json -size 32
popd