# Batterie

[English](battery.md) · **Français**

## Cellule testée

4000 mAh, 1S, 3C, 3,7 V nominal, 14,8 Wh. LOLIN C3 Pico, advertising à 100 ms,
battement RGB, 26 °C ambiants, aucune charge en dehors de la balise elle-même.

## Courbe de décharge complète

![Courbe de décharge](discharge_curve.png)

Deux passages sur la même cellule. Le test 1 était sa toute première décharge
après achat ; le test 2 a suivi un cycle complet de charge/décharge. L'écart de
52 % entre les deux est le résultat marquant — **dimensionnez sur le chiffre du
premier cycle si vous ne pouvez mesurer qu'une fois**, c'est le cas le plus
défavorable.

## Chronologie, test 2

| Écoulé | Tension | Événement |
|---|---|---|
| 0 | 4190 mV | charge pleine |
| 5 h 15 | 4030 mV | fin de la phase raide initiale, entrée en régime linéaire (~18 mV/h) |
| 35 h 59 | 3480 mV | **LED passe au jaune** — `BAT_LOW_MV` (3500 mV) |
| ~39 h 29 | ~3380 mV | coude de décharge — la pente passe de ~18 à ~140 mV/h |
| 40 h 13 | 3300 mV | **LED passe au rouge** — `BAT_CRIT_MV` (3300 mV) |
| 40 h 45 | 3200 mV | seuil de coupure franchi |
| 40 h 51 | ~3190 mV | **trois flashs rouges lents, deep sleep** — `BAT_CUTOFF_MV` |

Six minutes entre le franchissement du seuil de coupure et l'arrêt effectif :
le firmware exige trois relevés bas consécutifs, à une minute d'intervalle,
avant d'agir — voir [dépannage](troubleshooting.fr.md) pour la raison.

## Comment lire cette courbe

**Le régime linéaire occupe presque toute la décharge.** Sur le graphique en
largeur complète il paraît plat ; l'essentiel du comportement intéressant — le
coude, les deux changements de couleur de LED, la coupure — tient dans les
90 dernières minutes. Si vous ne prenez qu'une mesure pendant une chasse,
attendez-vous à tomber dans ce long plateau.

**Les segments de moins de six heures environ ne sont pas fiables**, près du
régime linéaire : les 10 mV de quantification de l'ADC dominent la lecture à
cette échelle. Dans ces données, deux segments courts consécutifs ont donné
28,6 et 13,8 mV/h ; combinés ils font 21,0, ce qui rejoint le régime de part et
d'autre. Fiez-vous aux fenêtres de plusieurs heures, pas à un segment isolé —
cela a coûté du temps réel pendant le test, avec un « coude » annoncé deux fois
sur des segments courts avant que le vrai n'apparaisse sur une fenêtre de six
heures.

**Ne convertissez pas la tension en pourcentage.** La relation n'est pas
linéaire et dépend de la cellule, de son âge et de sa chimie — c'est toute la
raison pour laquelle la balise émet des millivolts plutôt qu'un pourcentage
calculé. Le chiffre utile n'est pas « combien reste-t-il » mais **combien de
temps avant le prochain changement de couleur**, et c'est ce que donne ce
tableau.

## Dimensionnement

À environ 40 heures jusqu'à la coupure, une cellule de 4000 mAh couvre une
douzaine de chasses de trois heures. Le C3 Pico charge à 500 mA, donc recharger
une cellule de cette taille à vide demande plus de huit heures.

Une cellule de 1000 mAh couvre encore plusieurs chasses, se recharge en moins
de deux heures et occupe le quart du volume — ce qui compte beaucoup dès qu'on
dessine un boîtier destiné à se cacher sous un rocher. Voir
[matériel](../hardware/README.fr.md) pour l'arbitrage LiPo contre LiFePO4.
