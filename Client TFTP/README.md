# Client/Serveur TFTP

Ce programme peut envoyer et recevoir des fichiers à des serveurs TFTP.

## Utilisation

### Installation

```
Client TFTP % make tftp
```

### Envoi

Commande :
```
Client TFTP % ./tftp localhost put test.txt
```
Résultat :
``` 
Demande d'envoi de test.txt sur localhost:69...
Demande acceptée.
Paquet 1 envoyé.
Paquet 2 envoyé.
Paquet 3 envoyé.
[...]
Paquet 125 envoyé.
Paquet 126 envoyé.
Paquet 127 envoyé.
Le fichier test.txt a été téléversé avec succès.
```

### Téléchargement

Commande :
```
Client TFTP % ./tftp localhost get test.txt
```
Résultat :
```
Demande de test.txt sur localhost:69...
Paquet 1 reçu. (516 octets)
Paquet 2 reçu. (516 octets)
Paquet 3 reçu. (516 octets)
[...]
Paquet 215 reçu. (516 octets)
Paquet 216 reçu. (516 octets)
Paquet 217 reçu. (198 octets)
Le fichier test.txt a été téléchargé avec succès.
```

## Remarques

Le programme ne trouve pas toujours le serveur. C'est peut-être à cause d'une mauvaise configuration de la variable `hints`. (Pas assez spécifique ?) Fonctionne en localhost au bout de quelques tentatives...