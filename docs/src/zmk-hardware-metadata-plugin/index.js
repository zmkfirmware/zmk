var PrebuildPlugin = require("prebuild-webpack-plugin");
const fs = require("fs");
const yaml = require("js-yaml");
const glob = require("glob");

function checkCompatible(hardware, compatibleItem) {
  let compatible = false;

  if (hardware.compatible) {
    if (
      hardware.compatible === compatibleItem ||
      hardware.compatible.includes(compatibleItem)
    ) {
      compatible = true;
    }
  }

  // Check if compatibility is direct
  if (hardware.identifier === compatibleItem) {
    compatible = true;
  }

  return compatible;
}

function generateHardwareJson() {
  glob("../app/boards/**/*.yaml", (error, files) => {
    let hardwareList = [];

    files.forEach((filename) => {
      const contents = yaml.safeLoadAll(fs.readFileSync(filename, "utf8"));
      hardwareList = [...hardwareList, ...contents];
    });

    hardwareList = hardwareList.filter((h) => !h.hidden);

    let hardware = {
      standalone: hardwareList.filter((h) => h.standalone),
      compatible: [],
    };

    let compatibles = new Set(
      hardwareList
        .filter((h) => h.compatible)
        .map((h) => h.compatible)
        .flat()
    );

    for (let compatible of compatibles) {
      hardware.compatible.push({
        name: compatible,
        boards: hardwareList.filter(
          (h) => checkCompatible(h, compatible) && h.type === "mcu"
        ),
        shields: hardwareList.filter(
          (h) => checkCompatible(h, compatible) && h.type === "shield"
        ),
      });
    }

    fs.writeFileSync("src/data/hardware.json", JSON.stringify(hardware));
  });
}

module.exports = function () {
  return {
    name: "zmk-hardware-metadata-plugin",
    configureWebpack() {
      return {
        plugins: [
          new PrebuildPlugin({
            build: generateHardwareJson,
          }),
        ],
      };
    },
  };
};
