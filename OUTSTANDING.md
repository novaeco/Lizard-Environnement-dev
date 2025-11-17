# Travaux complémentaires éventuels

Les demandes initiales ont été implémentées : interpolation tapis chauffant, limites matière câbles/tapis, zones de Ferguson et distances UV avec avertissements, densités substrats, brumisation 3/7 jours, clavier AZERTY enrichi, persistance NVS, écran sécurité et CI `idf.py build`.

Pistes d'amélioration sur matériel réel :
- Mesurer les densités surfaciques tapis/câble avec thermomètre IR et affiner les coefficients par marque/modèle.
- Carthographier les UVI/UVA des lampes réellement utilisées (UVI-mètre) et enregistrer des profils spécifiques.
- Ajouter une action de réinitialisation NVS et, si besoin, un sélecteur de langue/clavier.
- Étendre les auto-tests avec valeurs lues sur capteurs externes lors d'une future intégration matériel.
# Travaux restants pour couvrir l'intégralité de la demande

Ce dépôt inclut l'ossature générale (modules de calcul, écrans LVGL, clavier AZERTY minimal). Les points ci-dessous restent à traiter pour coller strictement aux exigences initiales :

## Références normatives et données sourcées
- Ajouter dans le code et le README des références explicites (fiches fabricants, normes/guide FR-UE, courbes UVI/UVA, densités de substrat, débits de buses) et préciser pour chaque plage de valeurs si elle est normée ou prudente.
- Justifier les puissances surfaciques tapis/câble par matériau à partir de tableaux fabricants et de valeurs mesurées, avec marges de sécurité indiquées.

## Modèles de calcul
- Tapis chauffant : interpoler/ajuster la densité sur le tableau de calibration fourni (5 points), documenter la formule complète et valider les tensions 12/24 V par gamme fabricant.
- Câble chauffant : intégrer des limites surfaciques/linéiques par matériau, gestion des tensions (12/24/230 V) uniquement à titre théorique, et des espacements de spires pour éviter les points chauds.
- Éclairage 6500K/UVA/UVB : intégrer la distance réelle de la source, les zones de Ferguson (UVI cibles) avec avertissements, et des modèles basés sur des courbes de fabricants (irradiance vs distance) plutôt que des ratios simplifiés.
- Substrat : utiliser des densités issues de documents techniques (terreau, coco, sable, mélanges) avec plages min/max et marges de sécurité.
- Brumisation : baser les débits de buses sur datasheets, relier le nombre de buses à la surface/volume et à la typologie de milieu, dimensionner le réservoir pour plusieurs autonomies (3/7 jours) avec marges explicites.
- Ajouter des validations d’entrée plus strictes (dimensions minimales, puissances limites, distances UV) avec messages utilisateur détaillés.

## Interface LVGL
- Thème et ergonomie : finaliser le style moderne (palette, tailles de police, padding) pour 1024×600 et vérifier l’accessibilité tactile (tailles minimales, contrastes, focus visuel).
- Clavier : étendre l’AZERTY pour inclure accents, caractères spéciaux et bascule texte/chiffres par module; afficher uniquement quand nécessaire et gérer les masques de saisie numériques.
- Texte d’aide : ajouter sur chaque écran un rappel pédagogique des hypothèses et des plages sûres.

## Persistance et UX
- Implémenter la sauvegarde/rappel NVS des derniers paramètres saisis par module, avec option de réinitialisation.
- Ajouter un écran « À propos / sécurité » récapitulant les limites et renvoyant aux normes.

## Tests et CI
- Étendre les auto-tests (cas limites, entrées aberrantes) pour chaque module et ajouter une CI basique (idf.py build) pour verrouiller les régressions.

## Documentation
- Fournir la réponse complète structurée en 4 étapes (modélisation, architecture, code, README) dans le dépôt ou la documentation, avec exemples de commandes et avertissements explicites.
