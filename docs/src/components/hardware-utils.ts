import {
  Board,
  HardwareMetadata,
  Interconnect,
  Shield,
} from "../hardware-metadata";

export interface InterconnectDetails {
  interconnect?: Interconnect;
  boards: Board[];
  shields: Shield[];
}

export interface GroupedMetadata {
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
  shield.exposes?.forEach((id) => {
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

export function groupedMetadata(items: HardwareMetadata[]) {
  return items.reduce<GroupedMetadata>(
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
}
