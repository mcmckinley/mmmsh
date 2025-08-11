# PID 1 reaps zombies; better signal handling
# remove all Linux capabilities
# block gaining privs
# stop fork bombs
# resource caps
# immutable root FS
# give yourself a writable /tmp
# mount only your project

docker run --rm -it \
  --name c-shell \
  --init \
  --cap-drop=ALL \
  --security-opt no-new-privileges \
  --pids-limit=512 \
  --memory=1g --cpus=2 \
  --read-only \
  --tmpfs /tmp:exec,size=64m \
  -v "$PWD":/work \
  c-shell-dev
