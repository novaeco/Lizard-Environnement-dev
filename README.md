# TerrariumCalc-ESP32S3

Projet ESP-IDF v5.5 ciblant l'ESP32-S3 et la dalle Waveshare Touch LCD 7B (1024×600, contrôleur ST7262 + tactile GT911). L'application LVGL v9 fournit un calculateur complet des besoins matériels (tapis chauffant, éclairage LED, UV, substrat, brumisation) pour un terrarium, à partir des dimensions et paramètres saisis par l'utilisateur.

## Caractéristiques principales

- **Affichage RGB 16 bpp** via `esp_lcd_rgb_panel`, double tampon LVGL (1/10ᵉ d'écran en PSRAM).
- **Interface tactile GT911** sur I²C0 (SDA = GPIO8, SCL = GPIO9, IRQ = GPIO4).
- **Interface utilisateur LVGL v9** : champs de saisie validés, liste déroulante pour le matériau du plancher, bouton « Calculer » et affichage structuré des résultats.
- **Module de calcul dédié** (`components/calc`) encapsulant toutes les formules métier (tapis chauffant, éclairage, UV, substrat, brumisation).
- **Pilote GT911 minimal** (`components/gt911`) initialisant l’I²C maître, lisant les coordonnées et dégageant le statut tactile.
- **Architecture prête pour l’industrialisation** : configuration LVGL dédiée (`main/lv_conf.h`), code modulable, commentaires pour activer l’expandeur CH422G si nécessaire.

## Pré-requis

- ESP-IDF v5.5.x (`export.sh`/`export.ps1` chargé).
- Outilchain GCC 14 fournie par l’IDF.
- Carte ESP32-S3 disposant de PSRAM et connectée au module Waveshare Touch LCD 7B selon les broches définies dans `main/board_waveshare_7b.h`.

## Configuration & compilation

```bash
idf.py set-target esp32s3
idf.py fullclean
idf.py build
```

Pour flasher et lancer la console série :

```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

Remplacez `/dev/ttyUSB0` par le port série de votre cible. La configuration par défaut active la PSRAM, LVGL 16 bpp et les logs LVGL niveau INFO.

## Interface & usage

1. Allumez la dalle (optionnellement via l’expandeur CH422G si présent) et vérifiez que le rétroéclairage est alimenté.
2. Au démarrage, l’application crée l’UI LVGL : champs de saisie pour les dimensions (cm), l’épaisseur de substrat, la cible d’éclairement (lux), le rendement LED (lm/W), la puissance unitaire des LED, la cible UV, la sortie par module UV et la densité de brumisation (m²/buse).
3. Sélectionnez le matériau du plancher (bois, verre, PVC, acrylique) dans la liste déroulante.
4. Touchez « Calculer » pour exécuter le module `terrarium_calc_compute`. Les résultats détaillent :
   - Surface du sol, surface cible du tapis (~1/3), dimension carrée arrondie au 0,5 cm le plus proche, puissance brute corrigée par matériau puis valeur catalogue et caractéristiques électriques (V, A, Ω).
   - Flux lumineux total requis, puissance LED équivalente, nombre de modules LED (arrondi au supérieur).
   - Nombre de modules UV nécessaires pour atteindre l’intensité cible.
   - Volume de substrat en litres.
   - Nombre de buses de brumisation selon la densité fournie.
5. Modifiez les champs et relancez le calcul autant que nécessaire.

> ⚠️ Les valeurs sont indicatives : adaptez-les selon l’espèce, la régulation thermique réelle et les contraintes de sécurité.

## Structure du dépôt

```
TerrariumCalc-ESP32S3/
├── CMakeLists.txt
├── sdkconfig.defaults
├── README.md
├── components/
│   ├── calc/
│   │   ├── calc.c
│   │   ├── calc.h
│   │   └── CMakeLists.txt
│   └── gt911/
│       ├── gt911.c
│       ├── gt911.h
│       └── CMakeLists.txt
└── main/
    ├── app_main.c
    ├── board_waveshare_7b.h
    ├── lv_conf.h
    └── CMakeLists.txt
```

## Notes d’intégration

- Ajustez `board_waveshare_7b.h` si votre routage diffère (PCLK, HSYNC, DE, lignes RGB, GPIO rétroéclairage, etc.).
- La fonction `board_ch422g_enable()` peut être implémentée pour piloter un expandeur CH422G (EXIO6 pour LCD_VDD_EN, EXIO2 pour le backlight) si votre design le requiert.
- Les buffers LVGL (2 × 1/10ᵉ de l’écran) sont alloués en PSRAM ; assurez-vous que la PSRAM est opérationnelle (`CONFIG_SPIRAM=y`).
- Le pilote GT911 considère l’adresse I²C 0x14 (7 bits). Modifiez `gt911_config_t.i2c_address` si votre module utilise 0x5D.

## Validation attendue

- Compilation sans avertissements bloquants (`idf.py build`).
- Affichage de l’UI et interaction tactile fluide (lecture GT911).
- Calculs conformes aux formules spécifiées : densité 0,040 W/cm², coefficients matériaux, arrondi catalogue, conversions m² / litres, arrondis supérieurs pour LED, UV et buses.

Pour des tests unitaires supplémentaires, vous pouvez isoler `components/calc` et l’intégrer dans un projet d’hôte (Unity/CMock) afin de valider les cas limites des calculs.
