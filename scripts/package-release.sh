#!/usr/bin/env bash
# package-release.sh — gera um pacote de release reproduzível com checksum.
# Saída: release/<versao>/ com firmware.bin, littlefs.bin, SHA256SUMS, CHANGELOG.
set -euo pipefail

ENV="mentos-c3"
VERSION="${1:-0.1.0}"
OUT="release/${VERSION}"
BUILD=".pio/build/${ENV}"

echo ">> Build limpo..."
pio run -e "$ENV" -t clean || true
pio run -e "$ENV"
pio run -e "$ENV" -t buildfs

mkdir -p "$OUT"
cp "${BUILD}/firmware.bin" "$OUT/firmware.bin"
[ -f "${BUILD}/littlefs.bin" ] && cp "${BUILD}/littlefs.bin" "$OUT/littlefs.bin" || true

echo ">> Checksums..."
( cd "$OUT" && sha256sum ./* > SHA256SUMS )

cat > "$OUT/RELEASE_NOTES.md" <<EOF
# MentosArmyKnife-C3 ${VERSION}

- Data: $(date -u +%Y-%m-%dT%H:%M:%SZ)
- Ambiente: ${ENV}
- Firmware: firmware.bin
- Filesystem: littlefs.bin

## Verificação
\`\`\`
sha256sum -c SHA256SUMS
\`\`\`

## Recuperação / rollback
Veja docs/runbook.md (seção de gravação e recuperação por USB).
EOF

echo ">> Release em: $OUT"
cat "$OUT/SHA256SUMS"
