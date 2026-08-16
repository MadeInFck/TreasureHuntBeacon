# Treasure Hunt — Balise

[English](README.md) · **Français**

Spécification et fabrication d'une balise BLE pour l'application iOS
**Treasure Hunt**.

**Spécification de trame version 1.0.** Les balises et les versions de
l'application sont compatibles au sein d'une même version majeure.

Chaque étape d'une chasse peut être signalée par une balise iBeacon.
L'application mesure la proximité des balises pour guider les joueurs sur les
30 derniers mètres, là où le GPS seul est trop imprécis.

Deux possibilités :

- **Acheter** une balise iBeacon du commerce et la reconfigurer avec les valeurs
  ci-dessous.
- **Fabriquer** la vôtre à partir d'une carte ESP32 — voir
  [Fabriquer sa balise](#fabriquer-sa-balise).

---

## Spécification de la balise

C'est le contrat. Toute balise émettant la trame suivante sera reconnue par
l'application, quelle que soit sa provenance.

### Identifiants

| Champ | Valeur |
|---|---|
| **UUID** | `A4C27B10-5F3E-4E2A-9D8C-1B6F0E3D7A52` |
| **MAJOR** | Numéro de chasse, `1`–`65535` |
| **MINOR** | `(étape << 8) \| batterie` — voir ci-dessous |

L'UUID est **fixe pour tout le projet**. C'est un espace de noms, pas un
identifiant d'appareil : le modèle d'Apple prévoit un UUID par application, le
major et le minor portant le reste. Ne le modifiez pas — l'application filtre
sur cette valeur exacte et ne verra aucune balise émettant autre chose.

### Encodage du MINOR

Le minor, sur 16 bits, porte deux champs :

```
 15                    8 7                     0
┌───────────────────────┬───────────────────────┐
│     étape (1-255)     │   batterie (0-255)    │
└───────────────────────┴───────────────────────┘
```

| Octet | Signification |
|---|---|
| Haut | Numéro d'étape dans la chasse, `1`–`255` |
| Bas | Niveau de batterie, ou `0x00` pour « pas de télémétrie » |

Conversion de l'octet batterie en tension :

```
tension_mV = 2500 + (minor & 0xFF) * 10
```

Soit des pas de 10 mV de 2,50 V à 5,05 V, ce qui couvre LiPo comme LiFePO4.
`0x00` correspondrait à 2,50 V — une cellule déjà morte — d'où son usage sans
ambiguïté comme sentinelle « pas de télémétrie ».

Une balise du commerce ne peut pas produire cette télémétrie. Réglez son minor
sur `étape << 8` (par exemple `0x0100` = 256 pour l'étape 1) et l'application
affichera l'étape avec un niveau de batterie inconnu.

### Paramètres radio

| Paramètre | Valeur |
|---|---|
| Intervalle d'advertising | 100 ms |
| Flags d'advertising | `0x04` (BR/EDR non supporté) |
| Scan response | Désactivé |
| Champ TX power | **Mesuré par balise** — voir [calibration](docs/calibration.fr.md) |

L'octet TX power est le RSSI que lit un iPhone à exactement 1 mètre. Ce n'est
pas un réglage à recopier : une valeur fausse rend toutes les estimations de
distance fausses. Mesurez-la pour chaque exemplaire, dans son boîtier
définitif.

### Trame constructeur complète

25 octets, précédés du préfixe standard de longueur et de type `0x1A 0xFF` :

```
4C 00                                            Company ID Apple, little-endian
02 15                                            sous-type iBeacon + longueur restante
A4 C2 7B 10 5F 3E 4E 2A 9D 8C 1B 6F 0E 3D 7A 52  UUID, big-endian
00 01                                            MAJOR = 1, big-endian
01 00                                            MINOR = étape 1, sans télémétrie
C5                                               TX power = -59 dBm
```

Charge utile totale : 3 octets de flags + 27 octets de structure constructeur =
**30 des 31 octets disponibles**. Il ne reste pas de place pour un nom
d'appareil, d'où l'appel `BLEDevice::init("")` avec une chaîne vide dans le
firmware.


### LED de statut

Les cartes dotées d'une LED RGB annoncent leur identité au démarrage, une fois :

| Signal | Signification |
|---|---|
| 1 flash blanc | amorçage |
| N clignotements violets | numéro de chasse (MAJOR) |
| M clignotements bleus | numéro d'étape |

Puis une brève pulsation toutes les 5 s : **vert** vivante, **ambre** batterie
faible, **rouge** batterie critique. Le rouge n'apparaît nulle part ailleurs :
une pulsation rouge signifie toujours que la cellule demande attention.

On arme la balise, on compte les violets, on compte les bleus, on vérifie la
couleur de la pulsation, on referme. C'est tout le contrôle avant-vol, sans
téléphone.

---

## Utiliser une balise du commerce

La plupart des balises iBeacon du commerce se reconfigurent depuis
l'application de leur fabricant. Réglez :

- UUID → `A4C27B10-5F3E-4E2A-9D8C-1B6F0E3D7A52`
- Major → votre numéro de chasse
- Minor → `étape << 8` (étape 1 → `256`, étape 2 → `512`, étape 3 → `768`)
- Intervalle d'advertising → 100 ms si réglable

**À vérifier avant l'achat.** Certains modèles bon marché ont un UUID figé en
usine : ceux-là sont inutilisables. La fiche produit doit indiquer que l'UUID
est configurable par l'utilisateur.

---

## Fabriquer sa balise

| | |
|---|---|
| Firmware | [`firmware/beacon/`](firmware/beacon/) |
| Choix de carte et câblage | [`hardware/`](hardware/README.fr.md) |
| Boîtier | [`enclosure/`](enclosure/README.fr.md) |
| Calibration du TX power | [`docs/calibration.fr.md`](docs/calibration.fr.md) |
| Batterie et autonomie | [`docs/battery.fr.md`](docs/battery.fr.md) |
| Quand rien n'est détecté | [`docs/troubleshooting.fr.md`](docs/troubleshooting.fr.md) |

Le minimum pour mettre une balise en l'air : une carte ESP32 avec BLE, un câble
USB, et l'IDE Arduino avec le core ESP32 en version 3.x. La batterie et le
boîtier viendront ensuite.

---

## Organisation du dépôt

```
firmware/
  beacon/          Firmware de la balise — un flash par étape de chasse
  scanner/         Scanner de banc — décode les trames, utile en validation
  vbat_probe/      Sonde différentielle — localise un pont diviseur VBAT
  CONFIGURATION.md Réglages propres à chaque balise
hardware/          Cartes, schémas de câblage, nomenclature
enclosure/         Boîtier paramétrique OpenSCAD, STL prêts à trancher
docs/              Calibration, batterie, dépannage
```

Les commentaires du code sont en anglais uniquement, pour conserver une source
de vérité unique. La documentation existe en anglais et en français.

---

## Sécurité

Ce projet met en œuvre des cellules lithium. Lisez les avertissements de
[`hardware/README.fr.md`](hardware/README.fr.md) avant de câbler une batterie.
Un boîtier étanche laissé en plein soleil dépasse la plage de fonctionnement
sûre de la carte comme de la cellule.

---

## Licence

MIT — voir [LICENSE](LICENSE).
