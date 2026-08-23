#!/bin/sh
set -eu

package_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
key_file="$package_dir/keys/adriaan-de-groot-00ACD15E25A79FEE028B0EE57FEA3DA6169C77D6.asc"
key_sha256='9c651a9caee04c1315ce3bea2cb1ec5d8aa640b7a1ff9dc25acaeeb9798e11ca'
primary_fingerprint='00ACD15E25A79FEE028B0EE57FEA3DA6169C77D6'
signing_fingerprint='6D0837841C068A233F24127B14B6CC381BC256D6'

[ -r "$key_file" ] || {
  printf '%s\n' "No se encontró la clave pública verificada: $key_file" >&2
  exit 1
}

actual_sha256=$(sha256sum "$key_file" | awk '{print $1}')
[ "$actual_sha256" = "$key_sha256" ] || {
  printf '%s\n' 'El checksum de la clave pública no coincide.' >&2
  exit 1
}

created_keyring=false
if [ -z "${GNUPGHOME:-}" ]; then
  GNUPGHOME=$(mktemp -d)
  created_keyring=true
fi
export GNUPGHOME
umask 077
mkdir -p "$GNUPGHOME"
chmod 700 "$GNUPGHOME"

cleanup() {
  if [ "$created_keyring" = true ]; then
    rm -rf "$GNUPGHOME"
  fi
}
trap cleanup EXIT HUP INT TERM

gpg --batch --homedir "$GNUPGHOME" --import "$key_file"

gpg --batch --homedir "$GNUPGHOME" --with-colons --list-keys "$primary_fingerprint" \
  | awk -F: -v expected="$primary_fingerprint" '$1 == "fpr" && $10 == expected { found=1 } END { exit(found ? 0 : 1) }'
gpg --batch --homedir "$GNUPGHOME" --with-colons --list-keys "$signing_fingerprint" \
  | awk -F: -v expected="$signing_fingerprint" '$1 == "fpr" && $10 == expected { found=1 } END { exit(found ? 0 : 1) }'

cd "$package_dir"
makepkg --verifysource "$@"
