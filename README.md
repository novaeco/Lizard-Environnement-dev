# Calculateur Terrarium – ESP32-S3 Waveshare Touch LCD 7B

Projet ESP-IDF 6.1 + LVGL 9.4 conçu pour la carte Waveshare ESP32-S3-Touch-LCD-7B (1024×600, tactile capacitif). Il fournit une interface tactile complète pour dimensionner en sécurité les équipements d'un terrarium (tapis/câble chauffant, éclairage 6500K + UV, substrat, brumisation) sans piloter de GPIO.

## Fonctionnalités principales
- Écran paysage 1024×600 avec thème moderne et navigation par onglets.
- Clavier virtuel AZERTY FR qui s'affiche uniquement lors de la saisie des champs.
- Modules de calcul :
  - **Tapis chauffant** : surface ≈1/3 ajustable, puissance cataloguée (12/24 V) calibrée sur la table fournie.
  - **Câble chauffant** : longueur et densité surfacique contrôlée par matériau.
  - **Éclairage 6500K / UVA / UVB** : flux lumineux cible selon l'environnement (zones de Ferguson pour UVB/UVA).
  - **Substrat** : volume et masse selon densités typiques de matériaux.
  - **Brumisation** : nombre de buses et volume de réservoir avec marge de 20 %.
- Tests internes `heating_pad_run_self_test()` et assimilés pour vérifier les modèles.

## Arborescence
- `main/app_main.c` : initialisation écran/tactile/LVGL, démarrage UI, auto-tests.
- `main/calc_*.c|h` : modèles de calcul par domaine (tapis, câble, éclairage, substrat, brumisation) et hypothèses.
- `main/ui_main.c|h` : thème global et tabview principale.
- `main/ui_keyboard.c|h` : clavier virtuel AZERTY conditionnel.
- `main/ui_screens_*.c|h` : écrans LVGL par module avec saisie, calcul et rendu de résultats.
- `board_waveshare_7b.h` : brochage LCD/tactile (fourni).
- `sdkconfig.defaults` / `partitions.csv` : configuration 16 Mo flash, PSRAM 8 Mo.

## Construction et flash (ESP-IDF 6.1)
```bash
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

## Hypothèses et prudence
- Les formules sont conservatrices et s'appuient sur des plages issues de fiches fabricants, zones de Ferguson pour l'UV, valeurs surfaciques réalistes (≈0,04 W/cm² pour tapis sur verre), densités de substrats typiques. Elles fournissent des ordres de grandeur, pas une validation normative.
- Respecte les normes basse tension (NF C 15-100/CE), utilise des alimentations certifiées, protège les circuits (fusibles, différentiels), et vérifie la compatibilité thermique des matériaux (verre/PVC/PMMA sensibles aux points chauds).
- Les calculs UV supposent des mesures à distance donnée ; toujours vérifier avec un UVI-mètre avant usage avec des animaux.
- Aucun GPIO n'est piloté : le projet est purement calculateur. Toute commande matérielle future devra intégrer protections, relais et certifications adaptées.

## Points à compléter
- Ajouter des sources chiffrées directement citées (datasheets tapis/câbles, courbes UVI fabricants) dans les commentaires et dans la documentation pour justifier chaque plage retenue.
- Affiner la calibration du tapis chauffant en comparant la densité calculée aux cinq points du tableau fourni (interpolation/spline) et en validant sur du matériel réel avec thermomètre de surface.
- Étendre le clavier virtuel AZERTY pour couvrir les accents français et les caractères numériques enrichis, ainsi que la commutation rapide texte/nombre par écran.
- Introduire une persistance simple (NVS) des derniers paramètres saisis par module afin de faciliter les essais successifs, avec option de réinitialisation.
- Ajouter des auto-tests élargis (plages limites, entrées aberrantes) et une cible de CI basique (idf.py build) pour verrouiller la régression des modèles.

## Dépendances déjà prises en charge
- Drivers LCD RGB et tactile GT911 supposés opérationnels (flush LVGL et tick déjà intégrés).
- PSRAM requise pour le framebuffer (16 Mo flash / 8 Mo PSRAM sur le module Waveshare).

