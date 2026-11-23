#!/usr/bin/env bash
# End-to-end check using an isolated PostgreSQL database supplied by CI.
set -euo pipefail

: "${BORDERLAND_DB:?Set BORDERLAND_DB to a PostgreSQL connection string}"
BIN="${1:-./build-db/borderland_cli}"
test -x "$BIN"
mkdir -p ci-logs

# 1) Create a hero through the actual application and exit normally.
printf '1\nCI_Hunter\n0\n' | "$BIN" >ci-logs/01-create-hero.txt 2>&1
hero_id="$(psql "$BORDERLAND_DB" -v ON_ERROR_STOP=1 -Atc \
  "SELECT id FROM heroes WHERE name = 'CI_Hunter' ORDER BY id DESC LIMIT 1")"
[[ "$hero_id" =~ ^[0-9]+$ ]] || { echo 'Hero was not saved'; exit 1; }

# 2) Creating a hero should also create one progress record per contract.
contracts="$(psql "$BORDERLAND_DB" -v ON_ERROR_STOP=1 -Atc 'SELECT count(*) FROM contracts')"
progress="$(psql "$BORDERLAND_DB" -v ON_ERROR_STOP=1 -Atc \
  "SELECT count(*) FROM hero_contracts WHERE hero_id = $hero_id")"
[[ "$contracts" -ge 3 && "$progress" == "$contracts" ]] || {
  echo "Expected $contracts progress records, found $progress"; exit 1;
}

# 3) Reload the hero, accept the first quest, save changes and quit.
printf '2\n%s\n3\n1\n0\n' "$hero_id" | "$BIN" >ci-logs/02-accept-contract.txt 2>&1
status="$(psql "$BORDERLAND_DB" -v ON_ERROR_STOP=1 -Atc \
  "SELECT status FROM hero_contracts WHERE hero_id = $hero_id AND contract_id = 1")"
[[ "$status" == 'accepted' ]] || {
  echo "Contract did not persist as accepted (actual: $status)"; exit 1;
}

# 4) Reopen the saved hero and read its quest journal.
printf '2\n%s\n2\n0\n' "$hero_id" | "$BIN" >ci-logs/03-reload-hero.txt 2>&1
grep -Fq 'Принят' ci-logs/03-reload-hero.txt || {
  echo 'Accepted contract did not appear in reopened journal'; exit 1;
}

echo "PostgreSQL integration OK: hero=$hero_id, contracts=$contracts, persisted status=$status"
