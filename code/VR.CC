


#include <iostream>

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>



#define CamZ        100  // Distance camera-origine
#define DistZ       140  // Distance camera-ecran
#define EcranDroit   50.33 // Coin droit de l'ecran
#define EcranGauche -50.33 // Coin gauche
#define EcranHaut    50.33 // Coin superieur
#define EcranBas    -50.33 // Coin inferieur
#define INFINI      1e8 

using std::cout;

// Classes Point et Couleur


class Couleur {
public :

	int rouge, vert, bleu;
};



class Point {
public :

	float x, y, z;
};



// variables globales

#ifdef __MSDOS__
  unsigned char huge voxel[64][64][64]; // tableau contenant les voxels
#else
  unsigned char voxel[64][64][64];
#endif



Couleur map[256];      // Table des couleurs:  map[densite] => couleur
int ResX, ResY;      // Resolution en X et Y de l'ecran
Point O, D;          // O = origine, D = direction
FILE *FPTR ;         // pointeur au fichier
char fichier[100];   // nom du fichier image


// prototypes

void UsageMenu();
void ReadDataSet(char*);
void ReadCouleurMap();
void OuvreFichier();
void FermeFichier();
void Tracer();




// *************************************************************


int main(int argc, char *argv[])

{

// --- Verification des arguments ---

if ( (argc < 3) || (argc == 4) )   // pas le bon nombre d'arguments
	{
	UsageMenu();
	exit(0);
	}



if (argc == 5)
	{
	ResX = atoi(argv[3]);     // Assignation de la grosseur de l'image
	ResY = atoi(argv[4]);
	if ( (ResX < 1) || (ResY < 1) )
		{
		cout << "\n\nTaille de l'image non valide.\n";
		UsageMenu();
		exit(0);
		}
	}
else
	ResX = ResY = 100;      // Taille par defaut 100 X 100

strcpy(fichier, argv[2]);


cout << "\n---------------------------------------\n";
cout << "vr : volume rendering\n";
cout << "---------------------------------------\n";
cout << "\nnom du DATA SET : " << argv[1];
cout << "\nnom du fichier a creer : " << fichier;
cout << "\nresolution en x : " << ResX;
cout << "\nresolution en y : " << ResY << "\n\n";


cout << "Lecture du fichier de donnees...\n";

ReadDataSet(argv[1]); // Lecture du fichier de donnees

cout << "Lecture de la table couleur...\n";

ReadCouleurMap();  // Lecture de la table de couleur


cout << "Lance des rayons...\n";

Tracer();

cout << "\n--- Fin de l'execution de vr ---\n";



}// main()

// *************************************************************








// *************************************************************  
//
// UsageMenu(): Affiche le menu d'utilisation du programme
//
// *************************************************************  
void UsageMenu()

{

	cout << "\n--------------------------------------\n";
	cout << "vr : volume rendering\n";
	cout << "---------------------------------------\n\n";
	cout << "\nUsage: vr data_set image [resolution_x resolution_y]\n";
	cout << "data_set = nom du fichier de donnees\n";
	cout << "image    = nom du fichier de type RGB a creer\n";

}


// *************************************************************  
//
// Lecture du DataSet
//
// ReadDataSet
// *************************************************************  
void ReadDataSet(char* nom)
{
int i, j, k;
unsigned char tmp;


FILE *fichier;


if ( (fichier = fopen(nom, "rb")) != NULL)
{
// lire le header

tmp = ' ';

while(tmp != 0x0C)
	{
	tmp = getc(fichier);
	cout << tmp;
	}

// lire les donnees et les placer dans le tableau voxel

for(k=0; k < 64; k++)
	for(j=0; j < 64; j++)
		for(i=0; i < 64; i++)
		{
		if (fread(&voxel[i][k][j], sizeof(unsigned char), 1, fichier) != 1)
			{
			if(feof(fichier))
				cout << "\nfin du fichier prematuree!\n";
			else
				cout << "\nerreur lecture du fichier\n";
			exit(0);
			}

		#ifdef DEBUG0
		cout << "\nValeur du voxel [" << i;
		cout << ", " << j << ", " << k << "] = " << (int) voxel[i][j][k];
		#endif
		}


fclose(fichier);
}
else
    cout << "\n\nERREUR DE LECTURE DU DATA SET.";

} // ReadDataSet





// *************************************************************  
//
// Lecture de la table de couleur ( R G B Opacite )
//
// ReadCouleurMap
// *************************************************************  
void ReadCouleurMap()
{

int i, opacite, r, g, b;

FILE *fichier;

if ( (fichier = fopen("color.map", "r")) != NULL)
{

// lire les donnees et les placer dans le tableau map

for(i=0; i <= 255; i++)
	{
	fscanf(fichier, "%ud", &r);
	fscanf(fichier, "%ud", &g);
	fscanf(fichier, "%ud", &b);
	fscanf(fichier, "%ud", &opacite);

	map[i].rouge = r;
	map[i].vert  = g;
	map[i].bleu  = b;
	}

#ifdef DEBUG1
	for(i=0; i <= 255; i++)
	{
	cout << "\n\ni:" << i;
	cout << "   r: " << (int) map[i].rouge;
	cout << "   g: " << (int) map[i].vert;
	cout << "   b: " << (int) map[i].bleu;
	}
#endif


fclose(fichier);

}

else
    cout << "\n\nERREUR DE LECTURE DE LA TABLE COULEUR.";

} // ReadCouleurMap



// *************************************************************  
//
// Lance des rayons
//
// Tracer
// *************************************************************  
void Tracer()
{

  float IncX, IncY;      // increment pour pixel en X et Y
  int XX, YY = -1;
  float x, y;            // coordonnees du point sur l'ecran
  float w;               // projection de l'arete du cube sur l'ecran
  float xc, yc, zc = 32; // Point d'intersection du cube
  int i, j, k, px, py;
  float c, delx, dely, delz;   // norme, deltaX, deltaY, deltaZ
  float dx, dy, dz;
  unsigned char max;     // valeur maximale retournee par un rayon
  Couleur ligne[ResX];
  Couleur maxcol;
  char ch;
  char alpha = 0;


  IncX = (float) ((EcranDroit - EcranGauche) / float (ResX));
  IncY = (float) ((EcranHaut - EcranBas) / float (ResY));

  // cout << "\nIncX, IncY = " << (float) IncX << ", " << (float) IncY;


  // Pour chacune des lignes de l'ecran et chaque pixel,
  // on envoie un rayon a travers l'ecran et on regarde
  // si il intersecte le cube de donnee.  Si c'est le cas,
  // on prend la densite maximale des voxels ou le rayon
  // est passe.



  O.x = 0; O.y = 0; O.z = CamZ;  // Positionnement de la camera
  D.z = O.z - DistZ;             // Position en Z de l'ecran


  cout << "\nCreation du fichier\n";

  OuvreFichier();  // creation du fichier image RGB

  w = (32 * DistZ)/(O.z-32);  // limite perspective ecran-cube

  y = EcranHaut;

	do  // pour chaque lignes
	{

	  D.y = y;
	  max = 0;
	  XX = -1;
	  YY = YY + 1;
	  x = EcranGauche;


	  do // pour chaque pixels
	  {

	    XX = XX + 1;  // compteur d'indexation
	    D.x = x;

	    // est - ce que le rayon intersecte le plan du cube?

	    // non, on n'intersecte pas

	    if ( (fabs(D.x) >= w) || (fabs(D.y) >= w) )
	      ligne[XX] = map[0];  // pas d'intersection = couleur index 0
	    else
	    {

	      // oui, on intersect le cube ...
	      // on calcule la position du point d'intersection

	      yc = (D.y*(O.z-32)/DistZ);
	      xc = (D.x*(O.z-32)/DistZ);
	      zc = 32;

	      // dans quel voxel on commence le parcours?
	      if (yc < 0)
		j = (int) (31 - floor(yc));
	      if (yc >= 0)
		j =  (int) (32 - ceil(yc));
	      if (xc < 0)
		i = (int) (32 + floor(xc));
	      if (xc >= 0)
		i =  (int) (31 + ceil(xc));

	      k = 0; // 1ere tranche du cube...
	      max = voxel[i][j][k];  // 1ere valeur d'activite


	      // calcul du deplacement en x, y et z entre chaque voxel
	      c = sqrt(D.x*D.x + D.y*D.y + (D.z-O.z)*(D.z-O.z) );

	      if (D.x == 0)
	      {
		delx = 0;
		dx = INFINI;
	      }
	      else
		delx = fabs(c / D.x);

	      if (D.y == 0)
	      {
		dely = 0;
		dy = INFINI;
	      }
	      else
		dely = fabs(c / D.y);

	      delz = fabs(c / (D.z-O.z));

	      // determine l'increment des tableaux et la distance
	      // parcourue dans le voxel pour l'initialisation
	      if (D.x > 0)
	      {
		px = 1;
		dx = (floor(xc)+1-xc)*delx;
	      }
	      else
		if (D.x < 0)
		{
		  px = -1;
		  dx = (xc-floor(xc))*delx;
		}

	      if (D.y > 0)
	      {
		py = -1;
		dy = (floor(yc)+1-yc)*dely;
	      }
	      else
		if (D.y < 0)
		{
		  py = 1;
		  dy = (yc-floor(yc))*dely;
		}

	      dz = (floor(zc)+1-zc)*delz;

	      // parcours dans le cube et recherche de l'activite
	      // maximale

	      do
	      {
		
		if ( (dx <= dy) && (dx <= dz) )
		{
		  dx = dx + delx;
		  i = i + px;

		  if (max < voxel[i][j][k])
		    max = voxel[i][j][k];
		}

		if ( (dy <= dx) && (dy <= dz) )
		{
		 dy = dy + dely;
		 j = j + py;

		  if (max < voxel[i][j][k])
		    max = voxel[i][j][k];
		}

		if ( (dz <= dy) && (dz <= dx) )
		{
		  dz = dz + delz;
		  k = k + 1;

		  if (max < voxel[i][j][k])
		    max = voxel[i][j][k];
		}

	      } while ( (i > 0) && (i < 63) && (j > 0) && (j < 63) && (k > 0) && (k < 63) );

	      // ajoute la couleur selon la table

	      maxcol = map[max];

	      ligne[XX] = maxcol;

	    } // else

	    x = x + IncX;

	  } while( x < EcranDroit);


	  // mettre la ligne de couleur dans le fichier

	  cout << "\nLigne no " << YY << " terminee";

	  for (int loop=0; loop < ResX ; loop++)
	  {
	    fwrite(&alpha, sizeof(char), 1, FPTR);
	    fwrite(&(ch=(unsigned char)ligne[loop].bleu), sizeof(unsigned char), 1, FPTR);
	    fwrite(&(ch=(unsigned char)ligne[loop].vert ), sizeof(unsigned char), 1, FPTR);
	    fwrite(&(ch=(unsigned char)ligne[loop].rouge), sizeof(unsigned char), 1, FPTR);
	  }

	  y = y - IncY;

	} while( y > EcranBas);

  FermeFichier();

} // Tracer


// *************************************************************  
//
// Ouvrir fichier de sortie
//
// *************************************************************  
void OuvreFichier()
{
    char str[100];

    sprintf(str,"%s.rgb", fichier);
    cout << "nom du fichier de sortie : " << str;

    FPTR = fopen(str,"w+");

    if ( FPTR == NULL )
     {
     cout << "\n*** Erreur creation du fichier de sortie ***\n";
     exit(0);
     }

    fprintf(FPTR,"x:%d y:%d  type:RGBA \n\f", ResX, ResY);

} // OuvreFichier


// *************************************************************  
//
// Ferme le fichier de sortie
//
// *************************************************************  
void FermeFichier()
{
fclose(FPTR);
} // FermeFichier
