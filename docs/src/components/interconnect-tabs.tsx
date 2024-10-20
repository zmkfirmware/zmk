import Tabs from "@theme/Tabs";
import TabItem from "@theme/TabItem";

import { HardwareMetadata, Interconnect } from "../hardware-metadata";
import { groupedMetadata, InterconnectDetails } from "./hardware-utils";

interface InterconnectTabsProps {
  items: HardwareMetadata[];
}

function mapInterconnect(interconnect: Interconnect) {
  let content = require(`@site/src/data/interconnects/${interconnect.id}/design_guideline.md`);
  let imageUrl = require(`@site/docs/assets/interconnects/${interconnect.id}/pinout.png`);

  return (
    <TabItem value={interconnect.id} key={interconnect.id}>
      <img src={imageUrl.default} />

      <content.default />

      {interconnect.node_labels && (
        <>
          The following node labels are available:
          <ul>
            <li>
              GPIO: <code>&{interconnect.node_labels.gpio}</code>
            </li>
            {interconnect.node_labels.i2c && (
              <li>
                I2C bus: <code>&{interconnect.node_labels.i2c}</code>
              </li>
            )}
            {interconnect.node_labels.spi && (
              <li>
                SPI bus: <code>&{interconnect.node_labels.spi}</code>
              </li>
            )}
            {interconnect.node_labels.uart && (
              <li>
                UART: <code>&{interconnect.node_labels.uart}</code>
              </li>
            )}
            {interconnect.node_labels.adc && (
              <li>
                ADC: <code>&{interconnect.node_labels.adc}</code>
              </li>
            )}
          </ul>
        </>
      )}
    </TabItem>
  );
}

function mapInterconnectValue(interconnect: Interconnect) {
  return { label: `${interconnect.name} Shields`, value: interconnect.id };
}

function InterconnectTabs({ items }: InterconnectTabsProps) {
  let grouped = Object.values(groupedMetadata(items).interconnects)
    .map((i) => i?.interconnect as Interconnect)
    .filter((i) => i?.design_guideline)
    .sort((a, b) => a.id.localeCompare(b.id));

  return (
    <Tabs defaultValue={"pro_micro"} values={grouped.map(mapInterconnectValue)}>
      {grouped.map(mapInterconnect)}
    </Tabs>
  );
}

export default InterconnectTabs;
