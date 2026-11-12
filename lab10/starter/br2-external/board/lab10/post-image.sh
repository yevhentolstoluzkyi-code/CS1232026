#!/bin/sh
# Buildroot викликає цей скрипт після створення образів у output/images.
# Генеруємо start-qemu.sh, що запускає образ у системному QEMU
# (host-qemu у цьому дереві вимкнено — так збирання коротше).
set -eu

BINARIES_DIR="${BINARIES_DIR:-$1}"

cat > "${BINARIES_DIR}/start-qemu.sh" <<'SCRIPT'
#!/bin/sh
# Запуск образу ЛР 10. Вихід з QEMU: Ctrl-A, потім X.
cd "$(dirname "$0")"
ACCEL=""
if [ -w /dev/kvm ]; then
    ACCEL="-enable-kvm -cpu host"
fi
exec qemu-system-x86_64 -M pc -m 256 -smp 2 ${ACCEL} \
    -kernel bzImage \
    -drive file=rootfs.ext2,if=virtio,format=raw \
    -append "rootwait root=/dev/vda console=ttyS0" \
    -nographic \
    -nic user,model=virtio-net-pci,hostfwd=tcp:127.0.0.1:10022-:22 \
    "$@"
SCRIPT
chmod +x "${BINARIES_DIR}/start-qemu.sh"
