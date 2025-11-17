# Calculateur Terrarium – ESP32-S3 Waveshare Touch LCD 7B

Projet ESP-IDF 6.1 + LVGL 9.4 pour la carte Waveshare ESP32-S3-Touch-LCD-7B (1024×600, tactile capacitif). Interface tactile complète en paysage avec clavier AZERTY enrichi, persistance NVS et onglets de calcul (tapis/câble chauffant, éclairage 6500K-UVA-UVB, substrat, brumisation) + écran sécurité.

## Étape 1 – Modélisation & normes

- **Tapis chauffant** : interpolation linéaire sur la table fournie (densité catalogue ≈0,040-0,042 W/cm² sur 1/3 de surface). Correction hauteur (0,8-1,35×) et coefficient matériau. Limites matière : verre 0,055 W/cm², bois 0,065 W/cm², PVC 0,050 W/cm², acrylique 0,045 W/cm². Tension recommandée 12 V ≤18 W sinon 24 V. Sources : fiches fabricants tapis 12/24 V (0,03-0,06 W/cm²), bonnes pratiques terrariophiles.
- **Câble chauffant** : densité cible par matériau (verre 0,03-0,04 W/cm², bois 0,04-0,05 W/cm², PVC/PMMA 0,028-0,035 W/cm²). Longueur = max(densité cible, couverture serpentin pas 2,5-10 cm). Puissance linéique saisie (W/m), tension 12/24 V (230 V seulement à titre théorique). Avertissements si spires <3 cm ou densité >90 % de la limite. Sources : tableaux câbles chauffants terrarium/serre, recommandations SELV.
- **Éclairage 6500K / UVA / UVB** : lux cibles par milieu (8-20 klux). UVI cible via zones de Ferguson : zone 1 nocturne 0-1, zone 2 forêt 0,7-2, zone 3 tropical 1-3, zone 4 désert 3-6. Modèle 1/r² depuis la distance de référence fournie par l’utilisateur. Avertissements si UVI total >120 % ou <80 % de la plage. Sources : Ferguson et al., fiches fabricants UVB/UVA (irradiance à distance), guides herpétologiques.
- **Substrat** : densités techniques (kg/L) : terreau 0,65-0,85 ; fibre coco 0,45-0,65 ; mélange forestier 0,60-0,80 ; sable 1,5-1,7 ; sable/terre 1,0-1,3. Masse calculée en min/max/nominal. +10 % conseillé pour tassement. Sources : fiches horticoles et blocs coco 5 kg (70-80 L), données granulométriques sable.
- **Brumisation** : couverture par buse (0,08-0,16 m²) selon milieu. Débits typiques 60-120 mL/min par busette fine. Volume quotidien = débit × durée × cycles × nb buses. Réservoir = volume quotidien × autonomie (input) ×1,2, plus affichage direct pour 3 et 7 jours. Avertissement si densité de buses >10/m². Sources : datasheets MistKing/ExoTerra.

## Étape 2 – Architecture du projet

- `main/app_main.c` : initialisation LCD RGB, tactile GT911, LVGL, NVS, lancement UI et auto-tests des modèles.
- `main/calc_*.c|h` : calculs par domaine avec validations et auto-tests (heating_pad/cable, lighting, substrate, misting).
- `main/ui_*` : thème, clavier AZERTY FR évolué, écrans par module, onglet sécurité.
- `main/storage.c|h` : persistance NVS des dernières saisies par module.
- `board_waveshare_7b.h`, `sdkconfig.defaults`, `partitions.csv` : configuration cible ESP32-S3-WROOM-1-N16R8 (flash 16 Mo, PSRAM 8 Mo).
- `.github/workflows/ci.yml` : CI de build ESP-IDF 6.1 (`idf.py build`).

## Étape 3 – Code complet et points techniques

- Clavier AZERTY : accents (é,è,à,û,ç), bascule 123/ABC, filtrage numérique avec virgule/point. Affiché uniquement au focus des champs.
- UI LVGL : onglets Accueil, Tapis, Câble, Éclairage, Substrat, Brumisation, Sécurité. Cartes d’entrée avec aide pédagogique et avertissements (densité, UVI, 230 V, saturation brumisation).
- Persistance NVS : rechargement automatique des dernières valeurs saisies pour accélérer les itérations de dimensionnement ; sauvegarde après calcul.
- Auto-tests : fonctions `*_run_self_test()` exécutées au boot pour vérifier cohérence sur des cas nominal/limite (tableau tapis, UV désertique, réservoir 3/7 jours, etc.).

## Étape 4 – Compilation / flash / précautions

### Construction & flash (ESP-IDF 6.1)
```bash
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

### Précautions de sécurité
- Utilise exclusivement des alimentations SELV certifiées ; protège par fusible/disjoncteur adapté.
- Vérifie à l’infrarouge la température de surface des tapis/câbles avant présence animale ; reste sous les densités limites indiquées.
- Mesure l’UVI avec un appareil calibré et ajuste la distance des UVB/UVA pour rester dans la zone Ferguson de l’espèce.
- Prévois ventilation et bac de rétention pour la brumisation ; nettoie les buses pour éviter le colmatage.
- Ce projet ne commande aucun GPIO : il fournit des calculs indicatifs qui ne remplacent ni les normes complètes ni l’avis d’un professionnel.

