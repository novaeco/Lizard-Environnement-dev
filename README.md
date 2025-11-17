# Calculateur Terrarium – ESP32-S3 Waveshare Touch LCD 7B

## 1. Périmètre matériel et hypothèses conservatrices
- **Cible** : ESP-IDF 6.1 + LVGL 9.4 pour carte Waveshare ESP32-S3-Touch-LCD-7B (1024×600, tactile capacitif), module ESP32-S3-WROOM-1-N16R8 (flash 16 Mo, PSRAM 8 Mo). Sources d’horloge, LCD RGB et tactile GT911 sont initialisés dans `main/app_main.c` avec thème UI LVGL (`ui_main.c`).
- **Architecture** : modules de calcul (`main/calc_*.c|h`), écrans LVGL par domaine (`main/ui_screens_*.c|h`), clavier AZERTY (`ui_keyboard.*`), persistance NVS (`storage.*`), configuration matérielle (`board_waveshare_7b.h`, `sdkconfig.defaults`, `partitions.csv`). Aucun GPIO n’est piloté ; l’application fournit des dimensionnements et avertissements.
- **Hypothèses clés** :
  - Chauffage surfacique conservateur : limites matière 0,027-0,065 W/cm² selon matériau et hauteur (≥5 cm) avec interpolation monotone sur table catalogue [R1].
  - Câble chauffant : densité cible 0,028-0,050 W/cm² selon matériau, pas de spires recommandé ≥3 cm, limitation 230 V signalée [R2].
  - Éclairage : cibles lux 8-20 klux selon biotope, UVB via zones de Ferguson (UVI 0-6) et projection 1/r² depuis la distance de référence fournie [R3].
  - Substrats : densités typiques 0,45-1,7 kg/L selon matériau avec marge tassement +10 % [R4].
  - Brumisation : busette 0,08-0,16 m² et 60-120 mL/min, réservoir surdimensionné ×1,2 et projections autonomie 3/7 jours [R5].

## 2. Construction, flash et avertissements de sécurité
### Commandes ESP-IDF 6.1 (hôte Linux/Mac, `idf.py` dans le PATH)
```bash
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

### Avertissements critiques
- **SELV uniquement** : alimentations certifiées (NF C 15-100/CE), fusible/disjoncteur dédié, gaine thermo + repère polarité ; proscrire 230 V côté animal tant qu’aucune isolation double n’est certifiée.
- **Thermique** : vérifier à la caméra IR la densité surfacique des tapis/câbles avant présence animale ; ne jamais dépasser les plafonds matière [R1][R2]. Prévoir ventilation et contrôle de points chauds (PVC/PMMA sensibles).
- **UV** : mesurer l’UVI réel avec un radiomètre étalonné ; ajuster la distance des tubes/spot UVB pour rester dans la zone Ferguson visée [R3]. Éviter le regard direct humain/animal vers les sources UV.
- **Brumisation** : prévoir bac de rétention/vidange, nettoyage périodique des buses pour éviter le colmatage et la surpression ; séparer la logique de commande pompe/relais (non incluse) avec protections IP44 et anti-retour [R5].

## 3. Modules de calcul : références chiffrées & exemples d’utilisation
- **Tapis chauffant (`calc_heating_pad.*`)** — table catalogue 5-78 W sur 120-1 947 cm² (≈0,030-0,045 W/cm²) + plafonds matière : verre 0,055, bois 0,065, PVC 0,050, acrylique 0,045 W/cm² [R1]. Exemple : terrarium 80×40×25 cm en verre, ratio chauffé 0,33 → surface chauffée 1 056 cm², puissance arrondie 40 W (0,038 W/cm²) en 24 V avec alerte densité proche plafond si >90 %【F:main/calc_heating_pad.c†L13-L66】【F:main/calc_heating_pad.c†L88-L142】.
- **Câble chauffant (`calc_heating_cable.*`)** — densités recommandées 0,028-0,050 W/cm² (verre/PVC/bois) et pas ≥3 cm ; tension 12/24 V conseillée, 230 V signalé comme risque [R2]. Exemple : 120×50 cm bois, ratio 0,4, câble 15 W/m en 230 V, pas demandé 4 cm → zone chauffée 2 400 cm², longueur recommandée 7,2 m, densité 0,045 W/cm², alerte haute tension active【F:main/calc_heating_cable.c†L9-L94】.
- **Éclairage 6500K / UVA / UVB (`calc_lighting.*`)** — cibles lux par biotope : tropical 10-15 klux, désert 15-20 klux, tempéré 8-12 klux ; UVB via Ferguson : zone 1 (0-1 UVI), zone 2 (0,7-2), zone 3 (1-3), zone 4 (3-6) [R3]. Projection 1/r² entre distance de référence et distance cible. Exemple : bac 100×50×60 cm tropical, LED 1 500 lm /14 W, UVB 2,8 UVI @30 cm, UVA 0,12 mW/cm² @30 cm → 4 modules LED (~6 000 lm, ~12 klux), 2 modules UVB pour ~2,95 UVI total à 30 cm, distance recommandée 25-35 cm pour rester en zone 2-3【F:main/calc_lighting.c†L9-L120】.
- **Substrat (`calc_substrate.*`)** — densités typiques : coco 0,45-0,65 kg/L, forest blend 0,60-0,80, terreau 0,65-0,85, sable 1,50-1,70, sable/terre 1,00-1,30 [R4]. Exemple : 120×50 cm, couche 8 cm sable → volume 48 L, masse 76,8 kg (72,0-81,6 kg avec plage min/max), alerte si hauteur <5 cm【F:main/calc_substrate.c†L8-L75】.
- **Brumisation (`calc_misting.*`)** — couverture 0,08-0,16 m²/buse et débit 60-120 mL/min typique [R5]. Exemple : 120×50 cm tropical, buses 90 mL/min, cycles 2 min ×3/jour, autonomie 5 j → 6 buses, consommation 3,24 L/j, réservoir 19,44 L (3 j : 11,7 L ; 7 j : 27,2 L), alerte densité de buses si >10/m²【F:main/calc_misting.c†L9-L97】.

## 4. Interface, persistance et auto-tests
- **UI LVGL** : tabview (Accueil, Tapis, Câble, Éclairage, Substrat, Brumisation, Sécurité) dans `ui_main.c` et écrans dédiés `ui_screens_*.c`. Clavier virtuel AZERTY contextuel (`ui_keyboard.*`) avec bascule numérique et support des diacritiques. Thème réactif paysage 1024×600.
- **Persistance** : dernières saisies stockées en NVS par module (`storage.*`) pour accélérer les itérations de dimensionnement ; chargement au boot, sauvegarde après calcul.
- **Auto-tests** : chaque module expose `*_run_self_test()` (exécutés dans `app_main.c`) pour vérifier des cas nominal/limite (densité tapis/câble, UVB zone cible, réservoir 3/7 jours). Utiliser `idf.py monitor` pour inspecter les logs de test au démarrage.
- **Limites et durcissement** : l’application ne pilote aucun actionneur ; toute intégration matérielle doit ajouter relais protégés, inter-verrouillages thermiques, arrêt d’urgence et validation normative (CE, IP, double isolation). Conserver un UVI-mètre et une caméra IR pour audits réguliers.

**Références chiffrées** : [R1] Fiches Zoo Med ReptiTherm / Habistat 12/24 V (0,030-0,055 W/cm²) ; [R2] Exo Terra Forest/Desert Heat Cable 15-50 W (≈0,8-1,3 m·W/cm²) et limitation PVC/PMMA −10-20 % ; [R3] Ferguson et al., 2010 (zones UVI) + courbes Arcadia/Exo Terra T5 HO (1-1,5 UVI à 30 cm) ; [R4] NF U44-551 terreaux, blocs coco 5 kg (70-80 L), EN 13139 granulats siliceux ; [R5] MistKing/ExoTerra buses fines 0,08-0,12 L/h à 60 psi, volume réservoir = débit × temps × autonomie ×1,2.
