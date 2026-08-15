# Matériel

[English](README.md) · **Français**

## Choisir une carte

N'importe quel ESP32 doté du BLE émettra correctement. Ce qui les sépare, c'est
la quantité de circuiterie externe à ajouter pour un fonctionnement sur
batterie.

| Carte | Taille (mm) | Charge | Mesure VBAT | Antenne u.FL | Composants externes |
|---|---|---|---|---|---|
| M5Stack NanoC6 | 23,5 × 12 × 9,5 | non | non | non | 5 |
| XIAO ESP32C3 | 21 × 17,5 | oui | non | u.FL **uniquement** | 3 |
| XIAO ESP32C6 | 21 × 17,5 | oui | non | céramique + u.FL, bascule logicielle | 3 |
| Unexpected Maker TinyC6 | 35 × 17,8 × 4,3 | oui | **oui** | choisie à l'achat | 1 |
| ESP32 classique | variable | non | non | non | à garder comme scanner de banc |

Pour un prototypage sur USB uniquement, rien de tout cela n'a d'importance :
prenez ce que vous avez. Les différences apparaissent dès l'ajout d'une
batterie.

**À propos des antennes.** Le XIAO ESP32C6 bascule entre son antenne céramique
et le connecteur u.FL à l'exécution (GPIO3 au niveau bas active le commutateur
RF, GPIO14 au niveau haut sélectionne l'externe). Le choix reste donc ouvert
après impression du boîtier. L'ESP32C3 n'a aucune antenne intégrée : l'antenne
externe fournie est obligatoire, et un u.FL qui se déboîte dégrade la portée
sans panne visible.

## Alimentation

Une balise ne fonctionne que le temps d'une chasse, deux ou trois heures. Un
interrupteur en série sur le fil positif de la batterie fait disparaître
entièrement la question de la consommation de veille : interrupteur ouvert, le
courant est nul, ce qu'aucun firmware ne peut égaler.

Conséquence à connaître : interrupteur ouvert, la batterie est déconnectée du
chargeur, donc **la recharge est impossible switch sur off**.

### Option A — carte avec charge et mesure VBAT (TinyC6)

```
Batterie + ── Interrupteur ── BAT
Batterie − ─────────────────  GND
```

Trois composants. Charge, régulation et pont diviseur sont déjà sur le PCB.

### Option B — carte avec charge seule (XIAO C3/C6)

```
Batterie + ── Interrupteur ──┬── BAT+
                             │
                             R1 100k
                             ├────────── A0/A1  (ADC)
                             R2 100k     └─ C1 100nF ─ GND
                             │
                            GND
Batterie − ───────────────────  BAT−
```

Six composants. Piquez le pont **après** l'interrupteur, jamais avant : placé
en amont, il tire ~18 µA en permanence et vide la cellule en quelques semaines
alors que vous croyez tout coupé.

Placez C1 physiquement contre la carte, pas près des résistances. Son rôle est
de fournir en quelques microsecondes la charge que réclame l'échantillonneur de
l'ADC ; au bout d'un fil de 15 cm, l'inductance du câble annule l'effet.

### Option C — carte avec port Grove 5 V uniquement (NanoC6)

```
Batterie ── TP4056 ── Interrupteur ──┬── Boost MT3608 5V ── Grove 5V
(avec protection)                    │
                                     └── Pont R1/R2/C1 ──── Grove G1
                                                            Grove GND
```

Huit composants. Le boost n'existe que parce que le port Grove fournit du 5 V :
une LiPo à 3,7 V ne peut pas traverser un LDO et ressortir régulée à 3,3 V au
fil de sa décharge. Il en découle aussi une mesure batterie plus bruitée qu'en
option A ou B, pour des raisons purement topologiques.

**Couleurs des fils Grove :** noir GND, rouge 5 V, blanc G1, jaune G2.

## Nomenclature — option B

| Rep. | Composant | Remarque |
|---|---|---|
| BT1 | LiPo 1S, 500–1000 mAh | JST PH 2.0 |
| SW1 | Interrupteur à glissière SPST | sur le fil positif |
| R1, R2 | 100 kΩ 1 % | 0,25 W |
| C1 | 100 nF céramique | au plus près de la broche ADC |

## Avertissements

**Cellules lithium.** Un chargeur intégré est un chargeur LiPo, à tension
constante de 4,2 V. Une cellule LiFePO4 se charge à 3,6 V : la brancher sur un
chargeur LiPo revient à la surcharger. Si vous voulez la recharge par USB-C,
utilisez une LiPo.

**Réglez un MT3608 à 5,0 V avant de le connecter.** Ces modules sortent d'usine
avec le potentiomètre dans une position arbitraire et peuvent délivrer plus de
20 V. Alimentez-le seul, mesurez, ajustez, puis reliez-le à la carte.

**Utilisez un TP4056 avec protection.** Les modules nus n'exposent que B+/B−
sans coupure basse tension. Une LiPo descendue sous 2,5 V est bonne à jeter.

**Température.** Le NanoC6 est donné pour 0–40 °C. Un boîtier sombre et étanche
en plein soleil dépasse cette plage sans difficulté. Imprimez en filament clair,
placez les caches à l'ombre, et considérez une LiPo qui gonfle dans une boîte
fermée comme un sujet de sécurité, pas de longévité.

## Notes sur le boîtier

**Les tags NFC et le métal ne font pas bon ménage.** Un NTAG213 fonctionne par
couplage inductif à 13,56 MHz. Une poche LiPo ou un plan de masse de PCB juste
derrière désaccorde l'antenne et annule souvent complètement la portée de
lecture. Prévoyez 8 à 10 mm d'air entre le tag et l'électronique, sur une face
opposée à la cellule, ou utilisez un tag « on-metal » à couche de ferrite.

**Dégagement d'antenne.** Laissez 10 à 15 mm libres autour de l'extrémité
antenne du module, 1,5 mm d'épaisseur de paroi maximum devant, et aucun câblage
qui la traverse. Une antenne céramique est plus sensible au diélectrique proche
qu'une antenne gravée sur PCB.

**Hauteur de pose.** Le 2,4 GHz est absorbé par l'eau, donc par le sol, la roche
humide et le maquis. Une balise posée au sol est reçue beaucoup plus faiblement
et son estimation de distance devient erratique. Visez 30 à 50 cm de hauteur, et
calibrez à la hauteur d'usage réelle.
