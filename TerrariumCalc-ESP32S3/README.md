# TerrariumCalc-ESP32S3

## Objectif
TerrariumCalc-ESP32S3 fournit un calculateur interactif pour terrariums sur carte Waveshare Touch LCD 7B (1024×600, ST7262 + GT911) avec ESP-IDF v5.5 et LVGL 9. L'application permet de saisir les dimensions du terrarium et les paramètres lumineux/UV/microclimat afin d'estimer :

- Surface et puissance du tapis chauffant.
- Besoin en LED 6500 K selon un objectif de lux et le rendement (lm/W).
- Nombre de modules UVA/UVB.
- Volume de substrat en litres.
- Nombre de buses de brumisation.

Les calculs sont encapsulés dans le composant `calc` pour une réutilisation simple.

## Pré-requis
- ESP-IDF v5.5.x (`esp-idf` master >= 5.5) avec toolchain GCC 14.
- Carte ESP32-S3 disposant de la PSRAM activée.
- Écran Waveshare Touch LCD 7B (bus RGB 16 bits) et panneau tactile GT911 (I²C0).

## Configuration de l'environnement
```bash
idf.py set-target esp32s3
```

## Construction et flash
```bash
idf.py fullclean build
idf.py -p /dev/ttyACM0 flash monitor
```
Adaptez le port série selon votre système. `sdkconfig.defaults` active la PSRAM, les options LVGL nécessaires et optimise la taille du binaire.

## Interfaces
- **LVGL 9** (RGB565, double buffer en PSRAM) : interface professionnelle avec champs de saisie, listes déroulantes et bouton `Calculer`.
- **GT911** : driver minimal pour lecture du point tactile (I²C0, SDA=GPIO8, SCL=GPIO9, IRQ=GPIO4).

L'écran affiche un tableau de saisie à gauche et les résultats détaillés à droite. Chaque calcul est recalculé à la demande en appuyant sur `Calculer`. Le design privilégie la lisibilité sur un écran 1024×600.

## Structure du projet
```
TerrariumCalc-ESP32S3/
├── CMakeLists.txt
├── partitions.csv
├── sdkconfig.defaults
├── README.md
├── components/
│   ├── calc/
│   └── gt911/
└── main/
```

## Notes d'exploitation
- Les coefficients thermiques (bois, verre, PVC, acrylique) sont appliqués dans `components/calc`.
- Les valeurs catalogue des tapis chauffants suivent les puissances usuelles (5 à 100 W).
- Les densités et puissances (LED, UV, buses) sont configurables dans l'interface.
- Aucune sortie GPIO n'est commandée : seules les valeurs sont calculées et affichées.
- Les formules sont indicatives. Ajustez-les selon l'espèce et la conformité aux réglementations locales (CITES, CDC, I-FAP, etc.).

## Capture d'écran
Le rendu LVGL reprend le layout décrit ci-dessus. Après compilation et flash, effectuez une pression sur `Calculer` pour actualiser les résultats.

## Licence
MIT.
