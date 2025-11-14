# TerrariumCalc-ESP32S3

Projet ESP-IDF v6.1 ciblant l'ESP32-S3 et la dalle Waveshare Touch LCD 7B (1024×600, contrôleur ST7262 + tactile GT911). L'application LVGL v9.4 fournit un calculateur complet des besoins matériels (tapis chauffant, éclairage LED, UV, substrat, brumisation) pour un terrarium, à partir des dimensions et paramètres saisis par l'utilisateur.

## Caractéristiques principales

- **Affichage RGB 16 bpp** via `esp_lcd_rgb_panel`, double tampon LVGL (1/10ᵉ d'écran en PSRAM).
- **Interface tactile GT911** sur I²C0 (SDA = GPIO8, SCL = GPIO9, IRQ = GPIO4).
- **Interface utilisateur LVGL v9.4** : champs de saisie validés, liste déroulante pour le matériau du plancher, bouton « Calculer » et affichage structuré des résultats.
- **Module de calcul dédié** (`components/calc`) encapsulant toutes les formules métier (tapis chauffant, éclairage, UV, substrat, brumisation).
- **Pilote GT911 minimal** (`components/gt911`) initialisant l’I²C maître, lisant les coordonnées et dégageant le statut tactile.
- **Architecture prête pour l’industrialisation** : configuration LVGL dédiée (`main/lv_conf.h`), code modulable, commentaires pour activer l’expandeur CH422G si nécessaire.

## Pré-requis

- ESP-IDF v6.1.x (`install.sh` puis `export.sh` / `export.ps1` depuis l'IDF 6.1).
- Toolchain GCC 14 / LLVM fournie avec ESP-IDF 6.1 (support C gnu23 et C++ gnu++26).
- Carte ESP32-S3 disposant de PSRAM et connectée au module Waveshare Touch LCD 7B selon les broches définies dans `main/board_waveshare_7b.h`. Activez impérativement la PSRAM Octal 80 MHz dans la configuration (voir `sdkconfig.defaults`) pour éviter l'échec d'initialisation `quad_psram: PSRAM ID read error` constaté lorsque le mode Quad par défaut est appliqué.

### Versions logicielles embarquées

- **LVGL** : 9.4.x (configuration personnalisée dans `main/lv_conf.h`, build en mode FreeRTOS).
- **Pilotes ESP-IDF** : `esp_lcd_rgb_panel`, `esp_timer`, `esp_system` (API `esp_task_wdt`), `esp_driver_gpio`, `esp_driver_i2c`.
- **Module tactile** : pilote GT911 interne (`components/gt911`).
- **Module de calcul** : bibliothèque métier interne (`components/calc`) couverte par des tests Unity.
- **Gestion des dépendances** : `idf_component.yml` fige `lvgl/lvgl` sur la branche 9.4.x pour garantir la compatibilité API.

> Mettez à jour ces versions si vous migrez l'application afin de conserver une traçabilité logicielle complète.

## Configuration & compilation (ESP-IDF 6.1)

```bash
idf.py set-target esp32s3
idf.py fullclean
idf.py build
```

Lors du premier `idf.py build`, le gestionnaire de composants téléchargera automatiquement `lvgl/lvgl` en version 9.4.x telle que définie dans `idf_component.yml`.

Pour flasher et lancer la console série :

```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

Remplacez `/dev/ttyUSB0` par le port série de votre cible. La configuration par défaut active la PSRAM, LVGL 16 bpp et les logs LVGL niveau INFO.

### Points de configuration critiques

- `CONFIG_SPIRAM_TYPE_AUTO=y`, `CONFIG_SPIRAM_MODE_OCT=y` et `CONFIG_SPIRAM_SPEED_80M=y` forcent l'initialisation PSRAM Octal 80 MHz (indispensable sur ESP32-S3 + Waveshare 7B). Le mode Quad (`CONFIG_SPIRAM_MODE_SPI`) doit rester désactivé pour éviter l'erreur `PSRAM chip not found or not supported`.
- `CONFIG_SPIRAM_USE_MALLOC=y`, `CONFIG_SPIRAM_USE_CAPS_ALLOC=y` et `CONFIG_SPIRAM_TRY_ALLOCATE_WIFI_LWIP=y` permettent à LVGL d'allouer ses buffers en PSRAM tout en conservant des tampons de secours en SRAM interne.
- `CONFIG_LV_COLOR_DEPTH_16=y` doit être activé pour conserver l'interface en RGB565. Vérifiez aussi `CONFIG_LV_USE_LOG=y` et `CONFIG_LV_LOG_LEVEL_INFO=y` pour conserver un suivi runtime cohérent.
- `CONFIG_ESP_TASK_WDT_INIT=y` et `CONFIG_ESP_TASK_WDT_TIMEOUT_S=12` garantissent la compatibilité avec la reconfiguration du watchdog via `esp_task_wdt_reconfigure()` introduite dans l'IDF 6.x.
- `CONFIG_LOG_DEFAULT_LEVEL_INFO=y` et `CONFIG_I2C_ISR_IRAM_SAFE=y` facilitent le diagnostic matériel.

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
- L’adresse I²C du tactile GT911 est maintenant définie dans `BOARD_GT911_I2C_ADDR` (par défaut 0x14). Adaptez cette constante si votre dalle est câblée sur 0x5D sans modifier le code applicatif.

### Calibration tactile & dépannage

1. Vérifiez que `board_ch422g_enable()` alimente la dalle avant l'initialisation LCD (sinon écran noir).
2. Lancez l'application et observez les logs `gt911`: un message `GT911 product ID` doit apparaître.
3. Si le tactile semble inversé, ajustez `gt911_config_t.invert_x`, `gt911_config_t.invert_y` ou `gt911_config_t.swap_xy` dans `app_main.c`.
4. En cas d'absence totale d'évènements, validez le câblage IRQ (GPIO4) et SDA/SCL avec un oscilloscope puis activez `CONFIG_I2C_RECOVER_CLK_GPIO` pour forcer un reset bus.
5. Pour recalibrer LVGL, modifiez `gt911_config_t.logical_max_x/y` afin de correspondre exactement à la résolution 1024×600.

### Procédure de test automatisé

- `idf.py build flash monitor` : vérifie l'intégration complète sur cible.
- `idf.py build -T components/calc` : exécute les tests Unity de la bibliothèque de calcul.

Consultez `components/calc/test/test_calc.c` pour les cas limite couverts.

## Validation attendue

- Compilation sans avertissements bloquants (`idf.py build`) sous ESP-IDF 6.1 (warnings traités en erreurs par défaut).
- Affichage de l’UI et interaction tactile fluide (lecture GT911).
- Calculs conformes aux formules spécifiées : densité 0,040 W/cm², coefficients matériaux, arrondi catalogue, conversions m² / litres, arrondis supérieurs pour LED, UV et buses.

Pour des tests unitaires supplémentaires, vous pouvez isoler `components/calc` et l’intégrer dans un projet d’hôte (Unity/CMock) afin de valider les cas limites des calculs.

## Licence

Projet distribué sous licence **MIT**. Voir le fichier `LICENSE` pour le texte intégral. Les bibliothèques tierces conservent leurs licences respectives (LVGL, ESP-IDF).

## Résolution des incidents connus

- **PSRAM absente** : depuis la v1.1, l'application se met en pause après avoir désactivé le Task WDT et laisse la console ouverte avec le message « Système en pause ». Activez `CONFIG_SPIRAM` (ou désactivez-le si votre module n'en dispose pas) puis redémarrez ; vérifiez `esp_psram_get_size()` dans les logs avant de relancer.
- **Écran blanc ou tearing** : ajustez `BOARD_LCD_PIXEL_CLOCK_HZ` (>=18 MHz recommandé) et les timings HPW/HBP/HFP pour respecter la datasheet ST7262.
- **Watchdog LVGL** : si l'UI se fige, contrôlez les logs `LVGL watchdog tripped` et inspectez les traitements lourds dans `lv_timer`.
