# grabprobe — mod de diagnóstico (paso 1 del mod "agarre con QTE")

Graba la memoria de los dos luchadores en cada frame para localizar el **estado de agarre**,
su **temporizador** y la **vida**. Con esos datos se construye el mod real.

## Compilar

**Windows** (desde la raíz del repo, con las dependencias del juego ya instaladas):

```powershell
.\mods-src\grabprobe\build-windows.ps1
```

**Linux:**

```sh
cmake -S mods-src/grabprobe -B build/grabprobe -DCMAKE_BUILD_TYPE=Release
cmake --build build/grabprobe
```

Copia `grabprobe.dll` / `grabprobe.so` a la carpeta `mods/` que está junto al ejecutable.
En la página **Mods** del front-end debe aparecer activado.

> Desactiva el mod Tag Team mientras uses este: los dos usan el hook por frame y solo funciona uno.

## Cómo grabar (un archivo por movimiento)

1. Pelea 1P vs CPU (o vs 2P). Quédate quieto ~3 segundos.
2. Haz **un solo** agarre (Dragon Thunder, Absorber vida o un agarre normal) y deja que termine.
3. Quédate quieto ~3 segundos más y **mantén SELECT + R3** del mando 1 medio segundo.
4. Se crea `mods/grabprobe_000.bin` (luego `_001`, `_002`…). Cada uno guarda los últimos ~15 s.

Graba al menos: un agarre normal, un Dragon Thunder y un Absorber vida.

## Analizar

```sh
python3 mods-src/grabprobe/analyze.py mods/grabprobe_000.bin
```

Muestra los campos candidatos a estado de agarre, temporizador y vida, y la ventana de frames del agarre.
