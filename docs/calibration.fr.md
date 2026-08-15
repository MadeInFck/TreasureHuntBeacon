# Calibration du TX power

[English](calibration.md) · **Français**

L'octet `TX_POWER_1M` est la seule valeur qui transforme un RSSI brut en
mètres. C'est **le RSSI que lit un iPhone à exactement 1 mètre de la balise**.
Pas un réglage à recopier, pas un chiffre de fiche technique : une mesure.

Une valeur fausse rend fausses toutes les distances affichées par
l'application, d'une manière qui ressemble à un bug dans votre code de ranging.

## Procédure

À faire une fois par balise, dans son boîtier définitif.

1. **Balise dans son boîtier fermé**, batterie en place. Le plastique et une
   poche LiPo coûtent facilement 4 à 8 dB.
2. **À environ 1 m du sol**, sur un support non métallique, dégagée.
3. **iPhone à exactement 1 m**, tenu vertical, écran face à la balise, rien
   entre les deux et **votre corps hors de l'axe**. Un corps humain absorbe
   10 à 15 dB à 2,4 GHz.
4. **Relevez le RSSI brut pendant 30 secondes** et prenez la **médiane**, pas la
   moyenne. Une seule réflexion décale une moyenne.
5. Écrivez cette médiane dans `TX_POWER_1M`. Reflashez.
6. **Vérifiez** : l'application doit maintenant afficher environ 1,0 m à 1 m.

## Utilisez le RSSI brut, pas accuracy

`CLBeacon.accuracy` est *calculé à partir* du TX power que vous cherchez à
régler. Calibrer dessus est circulaire. Il vous faut `beacon.rssi`, en dBm.

Deux détails d'implémentation : `rssi` renvoie 0 quand iOS ne sait pas mesurer,
filtrez ces échantillons. Et le ranging tourne à environ 1 Hz avec une forte
dispersion, donc affichez une médiane glissante sur 10 relevés, sinon vous
lirez des valeurs qui sautent de ±10 dB sans savoir laquelle noter.

## Calibrez chaque exemplaire séparément

Deux cartes du même modèle divergeront de plusieurs dB. Si vous calibrez l'une
et recopiez la valeur sur l'autre, la seconde paraîtra systématiquement plus
loin à distance égale — et un enfant qui la cherche s'en apercevra.

Un écart supérieur à 5 dB environ entre deux exemplaires évoque un problème
matériel plutôt qu'une variation normale.

## Ce qu'on doit obtenir

Après calibration, attendez-vous à peu près à :

| Distance réelle | Affichée |
|---|---|
| 1 m | ~1 m |
| 3 m | 2 à 4 m |
| 10 m | entre 5 et 20 m |

**C'est normal.** La conversion est logarithmique : à courte distance quelques
dB font quelques centimètres, à 10 m les mêmes quelques dB font plusieurs
mètres. Un iBeacon n'est pas un instrument de mesure de distance. Au-delà de
5 m environ, l'estimation est au mieux indicative.

Calibrez pour la plage 0 à 3 m — celle où la recherche a réellement lieu — et
acceptez que le reste soit approximatif. Les longues distances sont le travail
de l'étage GPS.

## Affaiblissement sur le terrain

Le modèle de distance suppose un exposant d'affaiblissement. L'espace libre
vaut 2,0 ; une végétation dense se situe entre 2,5 et 3,5. Si votre application
expose ce paramètre, mesurez-le en extérieur dans le terrain que vous
utiliserez : il vous dira à quel point votre retour de proximité sera bruité,
avant de le découvrir avec des enfants sur les bras.
