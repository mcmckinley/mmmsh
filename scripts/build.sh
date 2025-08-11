docker build -t c-shell-dev \
  --build-arg UID=$(id -u) --build-arg GID=$(id -g) .
