#include <iostream>
#include <iomanip>
#include <vector>
#include <queue>
#include <string>
#include <random>
#include <chrono>
#include <algorithm>
#include <cctype>
#include <utility>

using namespace std;
using Horloge = chrono::steady_clock;

// Temps ecoule en microsecondes
double microsecondes(Horloge::time_point debut, Horloge::time_point fin) {
    return chrono::duration<double, micro>(fin - debut).count();
}

// =====================================================================
//  ABR (version iterative : pas de stack overflow, meme si hauteur = N)
// =====================================================================
class ABR {
private:
    struct Noeud {
        int valeur;
        Noeud *gauche, *droite;
        Noeud(int v) : valeur(v), gauche(nullptr), droite(nullptr) {}
    };
    Noeud* racine;

public:
    ABR() : racine(nullptr) {}

    ~ABR() {
        vector<Noeud*> pile;
        if (racine) pile.push_back(racine);
        while (!pile.empty()) {
            Noeud* n = pile.back();
            pile.pop_back();
            if (n->gauche) pile.push_back(n->gauche);
            if (n->droite) pile.push_back(n->droite);
            delete n;
        }
    }

    void inserer(int v) {
        Noeud** p = &racine;
        while (*p) {
            if (v < (*p)->valeur) p = &(*p)->gauche;
            else if (v > (*p)->valeur) p = &(*p)->droite;
            else return; // doublon ignore
        }
        *p = new Noeud(v);
    }

    bool rechercher(int v) const {
        Noeud* n = racine;
        while (n) {
            if (v == n->valeur) return true;
            n = (v < n->valeur) ? n->gauche : n->droite;
        }
        return false;
    }

    void supprimer(int v) {
        Noeud** p = &racine;
        while (*p && (*p)->valeur != v)
            p = (v < (*p)->valeur) ? &(*p)->gauche : &(*p)->droite;
        if (!*p) return;

        Noeud* n = *p;
        if (!n->gauche) {
            *p = n->droite;
            delete n;
        } else if (!n->droite) {
            *p = n->gauche;
            delete n;
        } else {
            // 2 enfants : on remplace par le minimum du sous-arbre droit
            Noeud** q = &n->droite;
            while ((*q)->gauche) q = &(*q)->gauche;
            Noeud* m = *q;
            n->valeur = m->valeur;
            *q = m->droite;
            delete m;
        }
    }

    // Hauteur = nombre de niveaux (arbre vide : 0, un seul noeud : 1)
    int hauteur() const {
        if (!racine) return 0;
        vector<Noeud*> niveau, suivant;
        niveau.push_back(racine);
        int h = 0;
        while (!niveau.empty()) {
            h++;
            suivant.clear();
            for (size_t i = 0; i < niveau.size(); i++) {
                if (niveau[i]->gauche) suivant.push_back(niveau[i]->gauche);
                if (niveau[i]->droite) suivant.push_back(niveau[i]->droite);
            }
            niveau.swap(suivant);
        }
        return h;
    }
};

// =====================================================================
//  Generation des ordres d'insertion (toujours les valeurs 1..N)
// =====================================================================
vector<int> ordreCroissant(int N) {
    vector<int> v;
    for (int i = 1; i <= N; i++) v.push_back(i);
    return v;
}

vector<int> ordreDecroissant(int N) {
    vector<int> v;
    for (int i = N; i >= 1; i--) v.push_back(i);
    return v;
}

vector<int> ordreAleatoire(int N, mt19937& gen) {
    vector<int> v = ordreCroissant(N);
    shuffle(v.begin(), v.end(), gen);
    return v;
}

// Mediane d'abord : donne l'arbre le plus bas possible (cas ideal)
vector<int> ordreEquilibre(int N) {
    vector<int> v;
    queue< pair<int,int> > file;
    file.push(make_pair(1, N));
    while (!file.empty()) {
        pair<int,int> r = file.front();
        file.pop();
        if (r.first > r.second) continue;
        int mid = r.first + (r.second - r.first) / 2;
        v.push_back(mid);
        file.push(make_pair(r.first, mid - 1));
        file.push(make_pair(mid + 1, r.second));
    }
    return v;
}

// Croissant avec 1 % de desordre (quelques echanges au hasard)
vector<int> ordrePresqueTrie(int N, mt19937& gen) {
    vector<int> v = ordreCroissant(N);
    int nbEchanges = max(1, N / 100);
    uniform_int_distribution<int> idx(0, N - 1);
    for (int k = 0; k < nbEchanges; k++) std::swap(v[idx(gen)], v[idx(gen)]);
    return v;
}

// =====================================================================
//  Mesure d'un cas
// =====================================================================
struct Resultat {
    string nom;
    int hauteur;
    double insertion;   // ms (total)
    double recherche;   // us (moyenne)
    double suppression; // us (moyenne)
    int trouves;
};

Resultat mesurer(const string& nom, const vector<int>& ordre, const vector<int>& valeursTest) {
    Resultat r;
    r.nom = nom;
    int nbOps = (int)valeursTest.size();
    ABR arbre;

    auto t0 = Horloge::now();
    for (size_t i = 0; i < ordre.size(); i++) arbre.inserer(ordre[i]);
    r.insertion = microsecondes(t0, Horloge::now()) / 1000.0;

    r.hauteur = arbre.hauteur();

    r.trouves = 0;
    t0 = Horloge::now();
    for (int i = 0; i < nbOps; i++) r.trouves += arbre.rechercher(valeursTest[i]) ? 1 : 0;
    r.recherche = microsecondes(t0, Horloge::now()) / nbOps;

    t0 = Horloge::now();
    for (int i = 0; i < nbOps; i++) arbre.supprimer(valeursTest[i]);
    r.suppression = microsecondes(t0, Horloge::now()) / nbOps;

    return r;
}

// =====================================================================
//  Saisie et affichage
// =====================================================================
int lireEntier(const string& message, int min, int max) {
    while (true) {
        cout << message;
        string ligne;
        getline(cin, ligne);
        try {
            int x = stoi(ligne);
            if (x >= min && x <= max) return x;
        } catch (...) {}
        cout << "  -> Valeur invalide (entre " << min << " et " << max << ").\n";
    }
}

bool ligneVide(const string& s) {
    return all_of(s.begin(), s.end(), [](unsigned char ch) { return isspace(ch) != 0; });
}

string sep1() {
    return "+" + string(28, '-') + "+" + string(11, '-') + "+" + string(19, '-')
         + "+" + string(19, '-') + "+" + string(19, '-') + "+\n";
}

void afficherTableauMesures(const vector<Resultat>& res) {
    cout << "\n" << sep1();
    cout << "| " << left << setw(27) << "Cas d'insertion"
         << "| " << setw(10) << "Hauteur"
         << "| " << setw(18) << "Insertion (ms)"
         << "| " << setw(18) << "Recherche (us)"
         << "| " << setw(18) << "Suppression (us)" << "|\n";
    cout << sep1();
    for (size_t i = 0; i < res.size(); i++) {
        cout << "| " << left << setw(27) << res[i].nom
             << "| " << right << setw(9) << res[i].hauteur << " "
             << "| " << setw(17) << fixed << setprecision(3) << res[i].insertion << " "
             << "| " << setw(17) << res[i].recherche << " "
             << "| " << setw(17) << res[i].suppression << " |\n";
    }
    cout << sep1();
}

int main() {
    cout << "=========================================\n";
    cout << "   ABR : insertion, recherche, suppression\n";
    cout << "   et hauteur selon l'ordre d'insertion\n";
    cout << "=========================================\n";

    cout << "\nMode :\n";
    cout << "  1. Automatique : tous les cas (croissant, decroissant, aleatoire,\n";
    cout << "                   equilibre, presque trie) avec les valeurs 1..N\n";
    cout << "  2. Manuel (un seul cas : vos valeurs, une par une)\n";
    int mode = lireEntier("Votre choix : ", 1, 2);

    mt19937 gen((unsigned)chrono::system_clock::now().time_since_epoch().count());
    vector<Resultat> resultats;
    int N = 0, NB_OPS = 0;

    if (mode == 1) {
        cout << "\nATTENTION : les cas croissant et decroissant sont en O(N^2).\n";
        cout << "Avec N = 100 000, comptez environ 20 secondes pour chacun.\n";
        N = lireEntier("Nombre d'elements N (1 - 100 000) : ", 1, 100000);
        NB_OPS = lireEntier("Nombre d'operations (recherche/suppression) par cas : ", 1, 1000000);

        // Memes valeurs a rechercher/supprimer pour tous les cas : comparaison juste
        uniform_int_distribution<int> val(1, N);
        vector<int> valeursTest(NB_OPS);
        for (int i = 0; i < NB_OPS; i++) valeursTest[i] = val(gen);

        cout << "\nMesures en cours (" << N << " elements, " << NB_OPS << " operations)...\n";
        cout << " - Aleatoire..." << flush;
        resultats.push_back(mesurer("Aleatoire", ordreAleatoire(N, gen), valeursTest));
        cout << " ok\n - Equilibre..." << flush;
        resultats.push_back(mesurer("Equilibre (mediane)", ordreEquilibre(N), valeursTest));
        cout << " ok\n - Presque trie (1% desordre)..." << flush;
        resultats.push_back(mesurer("Presque trie (1% desordre)", ordrePresqueTrie(N, gen), valeursTest));
        cout << " ok\n - Croissant..." << flush;
        resultats.push_back(mesurer("Croissant (1, 2, 3, ...)", ordreCroissant(N), valeursTest));
        cout << " ok\n - Decroissant..." << flush;
        resultats.push_back(mesurer("Decroissant (N, N-1, ...)", ordreDecroissant(N), valeursTest));
        cout << " ok\n";
    } else {
        vector<int> donnees;
        cout << "Entrez les nombres un par un (Entree apres chaque nombre).\n";
        cout << "Pour terminer : tapez ESPACE puis Entree (ou juste Entree).\n";
        while (true) {
            cout << "  element " << donnees.size() + 1 << " : ";
            string l;
            if (!getline(cin, l) || ligneVide(l)) break;
            try { donnees.push_back(stoi(l)); }
            catch (...) { cout << "  -> Nombre invalide, reessayez.\n"; }
        }
        if (donnees.empty()) {
            cout << "Aucun element saisi. Fin du programme.\n";
            return 0;
        }
        N = (int)donnees.size();
        NB_OPS = lireEntier("Nombre d'operations (recherche/suppression) : ", 1, 1000000);
        uniform_int_distribution<int> idx(0, N - 1);
        vector<int> valeursTest(NB_OPS);
        for (int i = 0; i < NB_OPS; i++) valeursTest[i] = donnees[idx(gen)];
        resultats.push_back(mesurer("Manuel (votre ordre)", donnees, valeursTest));
    }

    afficherTableauMesures(resultats);

    // Hauteur ideale = plus petit h tel que 2^h - 1 >= N
    long long hIdeale = 0;
    while (((1LL << hIdeale) - 1) < N) hIdeale++;

    cout << "\nParametres : N = " << N << " elements, " << NB_OPS << " operations par cas.\n";
    cout << "Hauteur ideale (arbre parfaitement equilibre) : " << hIdeale << "\n";
    cout << "(ms = millisecondes au total, us = microsecondes par operation)\n";

    bool toutTrouve = true;
    for (size_t i = 0; i < resultats.size(); i++)
        if (resultats[i].trouves != NB_OPS && mode == 1) toutTrouve = false;
    if (mode == 1)
        cout << "Verification : toutes les recherches ont reussi : " << (toutTrouve ? "oui" : "NON") << "\n";

    if (mode == 1) {
        string s2 =
            "+----------------------------+-----------+-----------+-----------+-----------+\n";
        cout << "\nComplexites attendues :\n" << s2;
        cout << "| " << left << setw(27) << "Cas d'insertion"
             << "| " << setw(10) << "Hauteur"
             << "| " << setw(10) << "Insertion"
             << "| " << setw(10) << "Recherche"
             << "| " << setw(10) << "Suppress." << "|\n" << s2;
        cout << "| " << setw(27) << "Aleatoire"
             << "| " << setw(10) << "~log N" << "| " << setw(10) << "N log N"
             << "| " << setw(10) << "log N" << "| " << setw(10) << "log N" << "|\n";
        cout << "| " << setw(27) << "Equilibre"
             << "| " << setw(10) << "log2(N+1)" << "| " << setw(10) << "N log N"
             << "| " << setw(10) << "log N" << "| " << setw(10) << "log N" << "|\n";
        cout << "| " << setw(27) << "Presque trie"
             << "| " << setw(10) << "grande" << "| " << setw(10) << "variable"
             << "| " << setw(10) << "variable" << "| " << setw(10) << "variable" << "|\n";
        cout << "| " << setw(27) << "Croissant"
             << "| " << setw(10) << "N" << "| " << setw(10) << "N^2"
             << "| " << setw(10) << "N" << "| " << setw(10) << "N" << "|\n";
        cout << "| " << setw(27) << "Decroissant"
             << "| " << setw(10) << "N" << "| " << setw(10) << "N^2"
             << "| " << setw(10) << "N" << "| " << setw(10) << "N" << "|\n" << s2;
        cout << "Note : la hauteur decide de tout : chaque operation coute au plus 'hauteur' comparaisons.\n";
    }

    return 0;
}