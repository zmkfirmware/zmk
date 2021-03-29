import React from "react";

import {
  Board,
  HardwareMetadata,
  Interconnect,
  Shield,
} from "../hardware-metadata";

interface HardwareListProps {
  items: HardwareMetadata[];
}

type ElementOrString = JSX.Element | string;

function itemHasMultiple(item: HardwareMetadata) {
  return (
    (item.type == "board" || item.type == "shield") &&
    (item.siblings?.length ?? 1) > 1
  );
}

function itemIds(item: HardwareMetadata) {
  if (item.type == "board" || item.type == "shield") {
    let nodes = (item.siblings ?? [item.id])
      .map<ElementOrString>((id) => <code key={id}>{id}</code>)
      .reduce(
        (prev, curr, index) => [...prev, index > 0 ? ", " : "", curr],
        [] as ElementOrString[]
      );
    return <span key={item.id}>{nodes}</span>;
  } else {
    return <code key={item.id}>{item.id}</code>;
  }
}

const TYPE_LABELS: Record<
  HardwareMetadata["type"],
  Record<"singular" | "plural", string>
> = {
  board: { plural: "Boards: ", singular: "Board: " },
  shield: { singular: "Shield: ", plural: "Shields: " },
  interconnect: { singular: "Interconnect: ", plural: "Interconnects: " },
};

function HardwareLineItem({ item }: { item: HardwareMetadata }) {
  return (
    <li>
      <a href={item.url}>{item.name}</a> (
      {TYPE_LABELS[item.type][itemHasMultiple(item) ? "plural" : "singular"]}{" "}
      {itemIds(item)})
    </li>
  );
}

interface InterconnectDetails {
  interconnect?: Interconnect;
  boards: Board[];
  shields: Shield[];
}

function mapInterconnect({
  interconnect,
  boards,
  shields,
}: InterconnectDetails) {
  if (!interconnect) {
    return null;
  }

  return (
    <div key={interconnect.id}>
      <h4>{interconnect.name} Keyboards</h4>
      {interconnect.description && <p>{interconnect.description}</p>}
      <h5>Boards</h5>
      <ul>
        {boards.map((s) => (
          <HardwareLineItem key={s.id} item={s} />
        ))}
      </ul>
      <h5>Shields</h5>
      <ul>
        {shields.map((s) => (
          <HardwareLineItem key={s.id} item={s} />
        ))}
      </ul>
    </div>
  );
}

interface GroupedMetadata {
  onboard: Board[];
  interconnects: Record<string, InterconnectDetails>;
}

function groupedBoard(agg: GroupedMetadata, board: Board) {
  if (board.features?.includes("keys")) {
    agg.onboard.push(board);
  } else if (board.exposes) {
    board.exposes.forEach((element) => {
      let ic = agg.interconnects[element] ?? {
        boards: [],
        shields: [],
      };
      ic.boards.push(board);
      agg.interconnects[element] = ic;
    });
  } else {
    console.error("Board without keys or interconnect");
  }

  return agg;
}

function groupedShield(agg: GroupedMetadata, shield: Shield) {
  shield.requires.forEach((id) => {
    let ic = agg.interconnects[id] ?? { boards: [], shields: [] };
    ic.shields.push(shield);
    agg.interconnects[id] = ic;
  });

  return agg;
}

function groupedInterconnect(agg: GroupedMetadata, item: Interconnect) {
  let ic = agg.interconnects[item.id] ?? { boards: [], shields: [] };
  ic.interconnect = item;
  agg.interconnects[item.id] = ic;

  return agg;
}

function HardwareList({ items }: HardwareListProps) {
  let grouped = items.reduce<GroupedMetadata>(
    (agg, hm) => {
      switch (hm.type) {
        case "board":
          return groupedBoard(agg, hm);
        case "shield":
          return groupedShield(agg, hm);
        case "interconnect":
          return groupedInterconnect(agg, hm);
      }
    },
    { onboard: [] as Board[], interconnects: {} }
  );

  return (
    <>
      <h2>Keyboards</h2>
      <h3>Onboard Controller Boards</h3>
      <p>
        Keyboards with onboard controllers are single PCBs that contain all the
        components of a keyboard, including the controller chip, switch
        footprints, etc.
      </p>
      <ul>
        {grouped["onboard"]
          .sort((a, b) => a.name.localeCompare(b.name))
          .map((s) => (
            <HardwareLineItem key={s.id} item={s} />
          ))}
      </ul>
      <h3>Composite Boards</h3>
      <p>
        Composite keyboards are composed of two main PCBs: a small controller
        board with exposed pads, and a larger keyboard PCB (a shield, in ZMK
        lingo) with switch footprints and location a where the controller is
        added.
      </p>
      {Object.values(grouped.interconnects).map(mapInterconnect)}
    </>
  );
}

export default HardwareList;
