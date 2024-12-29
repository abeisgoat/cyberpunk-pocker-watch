#!/usr/bin/bash
rm -rf generated
mkdir generated
convert ./mrdice_assets.png -crop 39x39+0+0   generated/die_six.png
convert ./mrdice_assets.png -crop 39x39+40+0  generated/die_four.png
convert ./mrdice_assets.png -crop 38x42+79+0  generated/die_eight.png
convert ./mrdice_assets.png -crop 40x41+0+40  generated/die_twelve.png
convert ./mrdice_assets.png -crop 39x45+40+40 generated/die_twenty.png

for i in {0..36..4}; do
  num=$(echo "${i} / 4" | bc)
  convert ./mrdice_assets.png -crop "4x8+${i}+108" "generated/font_${num}.png"
done

for i in {0..56..8}; do
  num=$(echo "${i} / 8" | bc)
  x=$(echo "1 + (${num} * 10)" | bc)
  convert ./Plump-Prose.png -background black -flatten  -crop "8x8+${x}+149" "generated/font_nb_${num}.png"
done

for i in {64..72..8}; do
  num=$(echo "${i} / 8" | bc)
  x=$(echo "1 + ((${num}-8) * 10)" | bc)
  convert ./Plump-Prose.png -background black -flatten  -crop "8x8+${x}+165" "generated/font_nb_${num}.png"
done

for i in {0..30..15}; do
  num=$(echo "${i} / 15" | bc)
  d=$(echo "(${num} + 2) * 2" | bc)
  convert ./mrdice_assets.png -crop "15x12+${i}+116" "generated/d${d}.png"
done

convert ./mrdice_assets.png -crop 20x12+45+116 generated/d12.png
convert ./mrdice_assets.png -crop 20x12+65+116 generated/d20.png

echo "#pragma once" > generated/assets.h
node tools/generate_pals >> generated/assets.h