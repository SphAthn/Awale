# Awale – Jeu d’Awalé en réseau (C)

Projet C implémentant le jeu traditionnel d’Awalé en mode **client / serveur** en ligne de commande.

## Structure des fichiers

- `awale.c`, `awale.h` : logique du jeu d’Awalé (plateau, coups, captures, statut).
- `server.c`, `server.h` : serveur TCP, gestion des clients, des parties, des commandes et des statistiques.
- `client.c`, `client.h` : client en ligne de commande, communication avec le serveur.
- `makefile` : compilation du serveur et du client.
- `stats.txt` : fichier texte contenant les statistiques des joueurs (créé/écrit par le serveur).

## Compilation

Depuis la racine du projet :

```bash
make
```

Cela génère deux exécutables :

- `server` : le serveur d’Awalé
- `client` : le client en ligne de commande

Pour nettoyer les fichiers objets et exécutables :

```bash
make clean
```

## Lancer le serveur

Dans un terminal :

```bash
./server
```

## Lancer un client

Dans un autre terminal (sur la même machine ou à distance) :

```bash
./client <adresse_serveur> <pseudo>
```

Exemples :

- Sur la même machine que le serveur :

  ```bash
  ./client 127.0.0.1 Alice
  ```

- Depuis une autre machine du réseau local (en remplaçant par l’IP du serveur) :

  ```bash
  ./client 192.168.1.10 Bob
  ```

Le client envoie d’abord le pseudo au serveur, puis attend les messages et les commandes en mode interactif.

## Commandes côté client

Une fois connecté, tu peux :

- Voir les utilisateurs connectés :

  ```
  /list
  ```

- Voir les parties en cours :

  ```
  /games
  ```

- Défi et gestion de défi :

  ```
  /challenge Bob
  /accept Alice
  /refuse Alice
  ```

- Jouer un coup pendant une partie :

  ```
  /move 3
  ```

  Interprétation des numéros :
  - Le joueur **Sud** voit ses 6 trous numérotés de 1 à 6 de gauche à droite.
  - Le joueur **Nord** voit également ses 6 trous de 1 à 6 de gauche à droite sur sa rangée.

- Observer une partie existante (l’ID est fourni par `/games`) :

  ```
  /observe 0
  /unobserve
  ```

- Abandonner une partie :

  ```
  /resign
  ```

- Afficher les statistiques :

  ```
  /stats
  ```

- Activer le mode démo (plateau préconfiguré pour une fin de partie rapide, utile pour les tests) :

  ```
  /demo
  ```

- Afficher l’aide complète :

  ```
  /help
  ```

## Fichier de statistiques `stats.txt`

Le serveur lit et écrit dans `stats.txt` pour conserver les statistiques des joueurs.
Le format est textuel, une ligne par joueur :

```text
pseudo parties_jouées victoires nuls défaites
```

Par exemple :

```text
Alice 10 6 2 2
Bob   8  3 1 4
```