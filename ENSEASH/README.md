# ENSEASH

Les questions 1 à 8 ont été traitées dans `enseash8.c`.

Il y a un fichier `.h` commun à tous les `.c` juste pour pouvoir réordonner les fonctions et faciliter la lecture.

Pour éviter les répétitions, les explications du code (en commentaire) ne sont pas reportées ici. 

Quelques commandes de test : 
```bash
% curl www.example.com > example.html
% wc -w < example.html
% curl www.example.com | sort > example.html
% echo hello world | wc -w | wc > test.txt
```

Cette version du programme ne sait pas ignorer les espaces entre gillemets ou echappés. Aussi, il ne semble pas fonctionner avec la commande 'tr' qui remplace des chaines dans une chaine...
