# Travaux complémentaires éventuels

Les demandes initiales ont été implémentées : interpolation tapis chauffant, limites matière câbles/tapis, zones de Ferguson et distances UV avec avertissements, densités substrats, brumisation 3/7 jours, clavier AZERTY enrichi, persistance NVS, écran sécurité et CI `idf.py build`.

Pistes d'amélioration sur matériel réel :
- Mesurer les densités surfaciques tapis/câble avec thermomètre IR et affiner les coefficients par marque/modèle.
- Carthographier les UVI/UVA des lampes réellement utilisées (UVI-mètre) et enregistrer des profils spécifiques.
- Ajouter une action de réinitialisation NVS et, si besoin, un sélecteur de langue/clavier.
- Étendre les auto-tests avec valeurs lues sur capteurs externes lors d'une future intégration matériel.
