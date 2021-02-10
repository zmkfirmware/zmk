import React from "react";
import hardware from "../data/hardware.json";

const compatibleLookUp = {
  pro_micro: "Pro Micro",
  proton_c: "Proton-C",
  makerdiary_m2: "makerdiary M.2",
};

export default function HardwareList() {
  return (
    <>
      <h3>Standalone Boards</h3>
      <ul>
        {hardware.standalone.map((p) => (
          <li key={p.identifier}>
            <a href={p.url}>{p.name}</a> (<code>{p.identifier}</code>)
          </li>
        ))}
      </ul>
      <span />
      {hardware.compatible.map((h) => (
        <React.Fragment key={h.name}>
          <span />
          <h3>{compatibleLookUp[h.name] || h.name}</h3>

          <h4>Boards</h4>
          <ul>
            {h.boards.map((p) => (
              <li key={p.identifier}>
                <a href={p.url}>{p.name}</a> (<code>{p.identifier}</code>)
              </li>
            ))}
          </ul>

          <h4>Shields</h4>
          <ul>
            {h.shields.map((p) => (
              <li key={p.identifier}>
                <a href={p.url}>{p.name}</a> (
                {p.components ? (
                  p.components.map((c, i) => (
                    <span key={p.identifier + p.name + c}>
                      <code>
                        {p.identifier}_{c}
                      </code>
                      <span>{i !== p.components.length - 1 ? ", " : null}</span>
                    </span>
                  ))
                ) : (
                  <code>{p.identifier}</code>
                )}
                )
              </li>
            ))}
          </ul>
        </React.Fragment>
      ))}
    </>
  );
}
