# Boîtier

[English](README.md) · **Français**

Boîtier étanche paramétrique pour une balise, en OpenSCAD. Ouvrez
`enclosure.scad`, réglez `part`, F6, exportez le STL. Les STL prêts à trancher
sont dans `stl/`.

## L'idée de conception

Le boîtier **ne comporte aucune traversée de paroi**. La LED se lit à travers
une zone amincie, l'interrupteur et l'USB-C restent à l'intérieur, et on ouvre
la boîte pour recharger. Le plan de joint du couvercle est donc le seul point à
étancher.

Un trou pour une LED d'état ou une prise de charge, c'est en général ce qui
fait fuir un boîtier imprimé. Ne pas en faire supprime le problème au lieu de
le résoudre.

## Dimensions

Les valeurs par défaut correspondent à une cellule de 4000 mAh mesurée à
65 × 40 × 12 mm et à une carte LOLIN C3 Pico.

| | |
|---|---|
| Encombrement | 101 × 76 × 35 mm |
| Cavité | 69 × 44 × 29 mm |

C'est volumineux, et la batterie en est la cause : la cellule impose la cavité,
le rebord d'étanchéité ajoute 14 mm, les oreilles de vissage 18 de plus. Une
cellule de 1000 mAh en 35 × 25 mm ramène l'ensemble autour de 70 × 60 mm — il
suffit de changer `bat_x`, `bat_y` et `bat_z`, tout le reste se recalcule.

**Mesurez votre cellule au pied à coulisse, connecteur JST et gaine
thermorétractable compris.** Ce faisceau dépasse souvent la poche de 2 à 3 mm
et c'est lui le vrai point haut.

## Ce que le modèle contient

- Gorge de joint de 2,5 × 1,8 mm, tout autour du rebord
- Quatre inserts M3 thermofusibles dans des bossages d'angle, reliés à la coque
- Lamages pour vis à tête cylindrique dans le couvercle
- Fenêtre LED : poche borgne prise par l'intérieur, laissant 0,6 mm de paroi
- Logette du tag NFC, sur la face externe par défaut (`nfc_inside = true` la
  bascule à l'intérieur et ajoute un cercle gravé marquant le point de tap)
- Berceau dans lequel le C3 Pico se pose — sans vis, donc sans avoir à deviner
  l'entraxe de perçage de la carte. Une mousse sous le couvercle la maintient.
- Nervures basses empêchant la cellule de glisser

## Joint

Coulez-le en place plutôt que d'acheter un O-ring. Remplissez la gorge de
silicone **neutre** (surtout pas acétique : l'acide acétique attaque le cuivre
et les soudures), enduisez la face du couvercle de vaseline comme agent de
démoulage, refermez, laissez 24 heures. Vous obtenez un joint moulé sur vos
propres tolérances d'impression, bien plus tolérant qu'un O-ring sur une gorge
imprimée en FDM où quelques dixièmes de dérive suffisent à créer une fuite.

## Impression

Les deux pièces face ouverte vers le bas, sans support. Le couvercle se pose
sur sa face externe, ergot vers le haut.

L'étanchéité vient de l'adhésion inter-couches, pas du joint :

- **5 périmètres** minimum, couches de 0,15 mm
- Température d'extrusion en haut de la plage recommandée, ventilation réduite
- Filament blanc ou naturel — une boîte sombre en plein soleil dépasse ce que
  le PLA tolère, et le blanc diffuse la LED bien mieux que le transparent, qui
  s'imprime laiteux et strié

Le PETG vaut mieux que le PLA si vous en avez : il ramollit vers 80 °C au lieu
de 60.

## Avant d'imprimer la pièce complète

Imprimez d'abord un **gabarit de rebord** — seulement les 5 mm supérieurs du
corps. Dix minutes, et vous saurez si `lip_gap` (0,35 mm par défaut) convient à
votre machine et si la gorge sort proprement. Trop serré, le couvercle ne
s'emboîte pas ; trop lâche, il vagabonde pendant le vissage.

## Points ouverts

L'interrupteur n'a pas encore de logement. Il reste 12 mm de hauteur libre
entre la carte et le couvercle, mais il flotte au bout de ses fils. À ajouter
une fois le modèle choisi.

L'orientation de l'antenne céramique dépend du bord du C3 Pico qui la porte. Le
berceau place la carte contre la paroi droite — vérifiez votre carte et inversez
le signe dans `board_tray()` si l'antenne se retrouve tournée vers l'intérieur
et vers la batterie.

## Respiration thermique

Une boîte totalement close qui passe de 20 à 55 °C au soleil voit sa pression
interne monter d'environ 12 %. En refroidissant la nuit, elle crée une
dépression qui aspire l'eau à travers le joint. C'est le mode de défaillance
classique des boîtiers étanches laissés dehors.

Sans objet pour une cache posée quelques heures. Si vous en laissez une
plusieurs jours, ajoutez une pastille d'équilibrage Gore-Tex collée à
l'intérieur sur un petit trou.
