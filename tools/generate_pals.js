#!/usr/bin/env node
const jimp = require("jimp");
const {cd, ls} = require('shelljs');
const {parse} = require("path");

const data={};
const files = ls('assets/*.png');


(async function() {
    for (const file of files) {
        const id = parse(file).name

        const lenna = await jimp.read(file);

        const width = lenna.getWidth();
        const height = lenna.getHeight();

        const datum = data[file] = {
            pal: [],
            pixels: new Array(width*height)
        }

        for (let y=0; y<height; y++) {
            for (let x=0; x<width; x++) {
                const rgba = jimp.intToRGBA(lenna.getPixelColor(x, y));
                const pixel = `0x${rgba.r.toString(16).toUpperCase()}, 0x${rgba.g.toString(16).toUpperCase()}, 0x${rgba.b.toString(16).toUpperCase()}`;
                let pal = datum.pal.indexOf(pixel);

                if (pal === -1) {
                    pal = datum.pal.length;
                    datum.pal.push(pixel);
                }

                const pixelPos = x + (y*width);
                datum.pixels[pixelPos] = pal;
            }
        }

        console.log(`
const uint8_t ${id}_width = ${width};
const uint8_t ${id}_height = ${height};

const uint8_t ${id}_pal[${datum.pal.length * 3}] = {
${datum.pal.join(",\n")}
};
       
const uint8_t ${id}_pixels[${datum.pixels.length}] PROGMEM = {`);

        for (let i = 0; i< datum.pixels.length; i+=10) {
            console.log(`${datum.pixels.slice(i, i+10).join(",")},`);
        }


console.log(`};`);
    }
})();
