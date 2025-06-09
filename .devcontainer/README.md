
Image build instructions:

```sh
TAG=latest
docker buildx build --platform linux/amd64 -t ghcr.io/mrexodia/riscyworkshop:$TAG .
docker push ghcr.io/mrexodia/riscyworkshop:$TAG
```