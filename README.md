# Sign dumper

Simple sign dumper for Minecraft worlds.

## Usage

```sh
find some_world -name *.mca | ./sign_dumper
```

## Build

Nix:
```sh
nix build
```

Docker:
```sh
docker build .
```

Host build:
```sh
make
```

## Version support

Should work for versions before 1.13, haven't really tested extensively so if
there are problems please create an issue about it and I'll take a look :3.

## Note

There are basically no error checking, so unsupported versions and/or corrupted
data ***WILL*** crash the program.

This will be fixed later :3.
