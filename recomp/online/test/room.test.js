// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of cps2recomp.
// The waiting room's countries: a chosen one is passed on to the others; none, or anything not two letters, is not.

import assert from "node:assert/strict";
import test from "node:test";
import { Room } from "../src/index.js";

test("a chosen country is passed on, in capitals; none or a bad one is not", async () => {
  const room = new Room({ storage: { async get() {}, async put() {}, async setAlarm() {} } }, {});
  const here = async (secret, more) => (await room.fetch(new Request("https://room/here", { method: "POST",
    body: JSON.stringify({ secret: secret.repeat(16), name: secret, games: ["sfa3"], protocol: 10, version: "v", ...more }) }))).json();
  await here("a", { country: "fr" });
  let seen = await here("b", {});
  assert.equal(seen.players[0].country, "FR");
  seen = await here("a", {});
  assert.equal(seen.players[0].country, undefined);
  for (const country of ["FRA", "1x", "<b", 7, null]) {
    await here("b", { country });
    assert.equal((await here("a", {})).players[0].country, undefined, JSON.stringify(country));
  }
});
