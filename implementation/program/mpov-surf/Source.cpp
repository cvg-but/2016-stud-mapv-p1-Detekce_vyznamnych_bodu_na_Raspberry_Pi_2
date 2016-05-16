/*Detekce vyznamnych bodu v obrazu pomoci metody SURF*/

#include <stdio.h>
#include <iostream>
#include <time.h>
#include <string.h>
#include "opencv2/core.hpp"
#include "opencv2/features2d.hpp"
#include "opencv2/opencv.hpp"
#include "opencv2/xfeatures2d.hpp"
#include "opencv2/highgui.hpp"

using namespace cv;
using namespace cv::xfeatures2d;

//globální promìnné
CvPoint rohyPredlohy[2];
int appMode = 0;
int pocetZiskanychBoduPredlohy = 0;
int vyp = 0;
int metoda = 0;
int init = 0;
int zmena = 0;

void obsluhaKlavesnice(int timeout)
{
	// cekej na vstup klavesnice
	const char key = cvWaitKey(timeout);

	switch (key) 
	{
		case 't': case 'T':
			if (appMode == 1) 
			{
				pocetZiskanychBoduPredlohy = 0;
			}
			else
				appMode = 1;
			break;
		case 'q': case 'Q':
			vyp = 1;
			break;
		case 'm': case 'M':
			zmena = 1;
			init = 1;
			appMode = 1;
			break;
	}
}

// funkce na obsluhu vstupu mysi
void obsluhaMysi(int event, int x, int y, int flags, void* params)
{
	if (appMode == 1) {
		rohyPredlohy[1] = cvPoint(x, y);
		switch (event) {
		case CV_EVENT_LBUTTONDOWN:
			rohyPredlohy[pocetZiskanychBoduPredlohy++] = cvPoint(x, y);
			break;
		case CV_EVENT_RBUTTONDOWN:
			break;
		case CV_EVENT_MOUSEMOVE:
			if (pocetZiskanychBoduPredlohy == 1)
				rohyPredlohy[1] = cvPoint(x, y);
			break;
		}
	}
}

void wait(int sekundy)
{
	clock_t konec;
	konec = clock() + sekundy * CLOCKS_PER_SEC;
	while (clock() < konec) {}
}

int identifikace(void)							//Detekce vlastnosti pripojene kamery
{
	VideoCapture cap(0);
	if (!cap.isOpened())
	{
		printf("Chyba inicializace kamery!!!\n");
		return -1;
	}
	wait(1);

	int jas, kontrast, saturace, expozice, odstin, zesileni, sirka, delka;
	jas = cap.VideoCapture::get(CV_CAP_PROP_BRIGHTNESS);
	kontrast = cap.VideoCapture::get(CV_CAP_PROP_CONTRAST);
	saturace = cap.VideoCapture::get(CV_CAP_PROP_SATURATION);
	expozice = cap.VideoCapture::get(CV_CAP_PROP_EXPOSURE);
	odstin = cap.VideoCapture::get(CV_CAP_PROP_HUE);
	zesileni = cap.VideoCapture::get(CV_CAP_PROP_GAIN);
	sirka = cap.VideoCapture::get(CV_CAP_PROP_FRAME_WIDTH);
	delka = cap.VideoCapture::get(CV_CAP_PROP_FRAME_HEIGHT);

	printf("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t\n", jas, kontrast, saturace, expozice, odstin, zesileni, sirka, delka);
	cap.release();
	return 0;
}

int main(void)
{

	printf("Vyberte detekcni metodu 0 - SURF, 1 - ORB\n");
	scanf("%i", &metoda);
	Mat img_1 = imread("e-fekt.png", CV_LOAD_IMAGE_GRAYSCALE);			//nacteni vyhledavaneho obrazku
	Mat img_2;

	//============Inicializace kamery============
	VideoCapture cap(0);
	if (!cap.isOpened())
	{
		printf("Chyba inicializace kamery!!!\n");
		return -1;
	}
	wait(1);				//cekani kvuli spravne inicializaci kamery

							//============Nastaveni hodnot kamery kamery============				//umozni nastavit parametry obrazu a pripadne nastaveni opakovat
	int stop = 1;
	int rozhodnuti;
	printf("Chcete nastavit parametry kamery?\t(0 = ne, 1 = ano, 2 = pouze rozliseni)\n");
	scanf("%i", &rozhodnuti);

	while (stop)
	{
		if (rozhodnuti == 1)												//Max a Min hodnoty jsou pro webkameru na stativu (pro jinou kameru lze zjistit hodnoty pomoci funkce "identifikace")
		{
			int sirka;
			int delka;
			int jas;
			int kontrast;
			Mat img_test;
			int opakovani;
			printf("Zadejte rozliseni obrazu:\t(napr. 640x480)\n");
			scanf("%ix%i", &sirka, &delka);
			cap.VideoCapture::set(CV_CAP_PROP_FRAME_WIDTH, sirka);					//nastaveni rozliseni (lze tim podstatne zrychlit detekci)
			cap.VideoCapture::set(CV_CAP_PROP_FRAME_HEIGHT, delka);

			printf("Zadejte hodnotu jasu:\t(0 - 255, default. 100)\n");				//nastaveni jasu
			scanf("%i", &jas);
			cap.VideoCapture::set(CV_CAP_PROP_BRIGHTNESS, jas);

			printf("Zadejte hodnotu kontrastu:\t(0 - 255, default. 27)\n");			//nastaveni kontrastu	
			scanf("%i", &kontrast);
			cap.VideoCapture::set(CV_CAP_PROP_CONTRAST, kontrast);

			cap >> img_test;
			imshow("MAPV", img_test);
			waitKey(0);
			destroyAllWindows();
			printf("Vyhovuje vam toto nastaveni?\t(0 = ne, 1 = ano)\n");
			scanf("%i", &opakovani);
			if (opakovani == 1)
			{
				stop = 0;
			}
			else
			{
				stop = 1;
			}
		}
		else if (rozhodnuti == 2)
		{
			int sirka;
			int delka;
			Mat img_test;
			int opakovani;
			printf("Zadejte rozliseni obrazu:\t(napr. 640x480)\n");
			scanf("%ix%i", &sirka, &delka);
			cap.VideoCapture::set(CV_CAP_PROP_FRAME_WIDTH, sirka);					//nastaveni rozliseni (lze tim podstatne zrychlit detekci)
			cap.VideoCapture::set(CV_CAP_PROP_FRAME_HEIGHT, delka);
			stop = 0;
		}
		else
		{
			stop = 0;
		}
	}

	//============Inicializace promenych============
	int minHessian = 1000;
	Ptr< SURF > detector = SURF::create(minHessian);
	Ptr< SURF > extractor = SURF::create();
	Ptr< ORB > orb_d = ORB::create(500, 2, 4, 15, 0, 1, ORB::HARRIS_SCORE, 31, 20);
	Ptr< ORB > orb_e = ORB::create();
	std::vector< KeyPoint > vyzn_body_1, vyzn_body_2, vyzn_body_2_zaloha;
	Mat deskriptor_1, deskriptor_2;

	const Ptr< flann::IndexParams >& indexParams = new flann::KDTreeIndexParams(5);			//The number of parallel kd-trees to use. Good values are in the range [1..16]
	const Ptr< flann::SearchParams >& searchParams = new flann::SearchParams(10);			//kolikrat se bude prochazet seznam bodu

	int i = 0;
	double vzdalenost;
	int kolo = 0;
	double min_vzdalenost = 10000000000;
	double min_vzdalenost10;
	int citac = 0;

	//============Preprocesing obrazu============					zkousel jsem hranovat obraz ale je to pak strasne pomaly a nepresny
	//Mat img_1;
	//Canny(obraz, img_1, 50, 250, 3);				//cannyho detektor hran
	//adaptiveThreshold(obraz, img_1, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 5, 10);	//adaptivni hranovy detektor

	//============Vypocet vyznamnych bodu pro vyhledavany obraz============

	//============Inicializace okna============
	cvNamedWindow("MAPV", CV_WINDOW_AUTOSIZE);
	cvSetMouseCallback("MAPV", obsluhaMysi, NULL);

	if (metoda == 0)
	{
		detector->detect(img_1, vyzn_body_1);
		extractor->compute(img_1, vyzn_body_1, deskriptor_1);
	}
	else
	{
		orb_d->detect(img_1, vyzn_body_1);
		orb_e->compute(img_1, vyzn_body_1, deskriptor_1);
	}
	//============Vypocet pozice rohu vyhledavaneho obrazu============
	std::vector< Point2f > img_1_rohy(4);
	img_1_rohy[0] = cvPoint(0, 0);
	img_1_rohy[1] = cvPoint(img_1.cols, 0);
	img_1_rohy[2] = cvPoint(img_1.cols, img_1.rows);
	img_1_rohy[3] = cvPoint(0, img_1.rows);

	//============Vypocet vyznamnych bodu pro vyhledavany obraz============
	while (1)
	{
		if (appMode == 0)
		{
			//============Nacteni snimku kamery============
			clock_t zacatek = clock();
			Mat snimek;

			cap >> snimek;
			cvtColor(snimek, img_2, CV_RGB2GRAY);

			if (!img_1.data || !img_2.data)
			{
				printf("Chyba nacteni obrazu!!!\n");								//kontrola nacteni obou obrazu
				return -1;
			}

			//============Preprocesing sceny============
			//Mat img_2;
			//Canny(scena, img_2, 50, 250, 3);				//cannyho detektor hran
			//adaptiveThreshold(scena, img_2, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 5, 10);		//adaptivni hranovy detektor

			//============Vypocet vyznamnych bodu pro scenu============
			if (metoda == 0)
			{
				detector->detect(img_2, vyzn_body_2);
				extractor->compute(img_2, vyzn_body_2, deskriptor_2);
			}
			else
			{
				orb_d->detect(img_2, vyzn_body_2);
				orb_e->compute(img_2, vyzn_body_2, deskriptor_2);
			}

			//============Osetreni chyby pri zakryte kamere============
			if (vyzn_body_2.size() == 0 && kolo == 0)
			{
				printf("Rozsvitte prisvit nebo presunte snimany objekt na svetlo\n");
				return -1;
			}

			if (vyzn_body_2.size() == 0 && kolo == 1)
			{
				vyzn_body_2 = vyzn_body_2_zaloha;						//udrzuje posledni vyzn. body pokud je zakryta kamera, dokud kamera neco nezachyti
			}
			else
			{
				vyzn_body_2_zaloha = vyzn_body_2;
			}
			kolo = 1;

			//============Porovnani vyzn. bodu============
			if (deskriptor_1.type() != CV_32F)
			{
				deskriptor_1.convertTo(deskriptor_1, CV_32F);
			}
			if (deskriptor_2.type() != CV_32F)
			{
				deskriptor_2.convertTo(deskriptor_2, CV_32F);
			}

			FlannBasedMatcher matcher(indexParams, searchParams);
			std::vector< DMatch > shody;

			if ((deskriptor_1.type() == deskriptor_2.type()) && (deskriptor_1.cols == deskriptor_2.cols))
			{
				matcher.match(deskriptor_1, deskriptor_2, shody);
			}

			//============Vyber dobrych shod============

			if ((min_vzdalenost) == 0)
			{
				min_vzdalenost = 10000000000;							//pokud se min_vzdalenost dostala na nulu tak se tam zasekla a program prestal vykreslovat
			}

			if (citac < 8)
			{
				min_vzdalenost = 10000000000;							//stejne jako predchozi if slouzi k zamezeni ztrat spojenych bodu
			}

			std::vector< DMatch > dobre_shody;

			for (i = 0; i < deskriptor_1.rows; i++)
			{
				vzdalenost = shody[i].distance;
				if (vzdalenost < min_vzdalenost)
				{
					min_vzdalenost = vzdalenost;
				}
			}

			for (i = 0; i < deskriptor_1.rows; i++)
			{
				if (shody[i].distance < (min_vzdalenost * 2))
				{
					dobre_shody.push_back(shody[i]);
				}
			}

			citac = dobre_shody.size();

			//============Vykresleni shod============
			Mat img_shody;
			drawMatches(img_1, vyzn_body_1, img_2, vyzn_body_2, dobre_shody, img_shody, Scalar::all(-1), Scalar::all(-1), std::vector< char >(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);

			//============Nalezeni okraju objektu============
			std::vector< Point2f > img_1_velikost;
			std::vector< Point2f > img_2_velikost;

			for (i = 0; i < dobre_shody.size(); i++)
			{
				img_1_velikost.push_back(vyzn_body_1[dobre_shody[i].queryIdx].pt);
				img_2_velikost.push_back(vyzn_body_2[dobre_shody[i].trainIdx].pt);
			}

			if (img_1_velikost.size() > 6)											//pri hodne male hodnote nema kreslici funkce dostatek dat a vyhodi chybu, proto je v tom pripade kresleni obdelniku preskoceno
			{
				Mat G;
				G = findHomography(img_1_velikost, img_2_velikost, CV_RANSAC);
				
				//============Vykresleni okraju objektu============
				std::vector< Point2f > img_2_rohy(4);
				if (G.dims != 0)						//obcas proste predchozi funkce (findHomography) nic nenasla a program spadl
				{
					perspectiveTransform(img_1_rohy, img_2_rohy, G);
					line(img_shody, img_2_rohy[0] + Point2f(img_1.cols, 0), img_2_rohy[1] + Point2f(img_1.cols, 0), Scalar(0, 255, 255), 4);
					line(img_shody, img_2_rohy[1] + Point2f(img_1.cols, 0), img_2_rohy[2] + Point2f(img_1.cols, 0), Scalar(0, 255, 255), 4);
					line(img_shody, img_2_rohy[2] + Point2f(img_1.cols, 0), img_2_rohy[3] + Point2f(img_1.cols, 0), Scalar(0, 255, 255), 4);
					line(img_shody, img_2_rohy[3] + Point2f(img_1.cols, 0), img_2_rohy[0] + Point2f(img_1.cols, 0), Scalar(0, 255, 255), 4);
				}
			}

			//============Vypocet rychlosti cele smycky============
			clock_t konec = clock();
			double fps = 0;
			double rozdil = konec - zacatek;
			fps = 1 / (rozdil / 1000);

			std::ostringstream stre;
			stre << fps;
			std::string tisk4 = stre.str();

			std::ostringstream stra;
			stra << (rozdil / 1000);
			std::string tisk3 = stra.str();

			char tisk1[] = "cas ";
			char tisk2[] = "fps ";

			//============Vypsani hodnot do obrazu============
			Point pocatek_textu1(0, img_shody.rows - 40);
			putText(img_shody, tisk1, pocatek_textu1, FONT_HERSHEY_PLAIN, 3, Scalar(0, 0, 255), 2, 5);		//cas
			Point pocatek_textu2(0, img_shody.rows);
			putText(img_shody, tisk2, pocatek_textu2, FONT_HERSHEY_PLAIN, 3, Scalar(0, 0, 255), 2, 5);		//fps
			Point pocatek_textu3(100, img_shody.rows - 40);
			putText(img_shody, tisk3, pocatek_textu3, FONT_HERSHEY_PLAIN, 3, Scalar(0, 0, 255), 2, 5);		//hodnota casu
			Point pocatek_textu4(100, img_shody.rows);
			putText(img_shody, tisk4, pocatek_textu4, FONT_HERSHEY_PLAIN, 3, Scalar(0, 0, 255), 2, 5);		//hodnota fps
			imshow("MAPV", img_shody);

			//pøíkazy z klávesnice
			obsluhaKlavesnice(10);

			if (vyp == 1)
			{
				break;
			}

		}
		else
		{
			if (zmena == 0)
			{
				//ukazat shody
				Mat vyber = img_2.clone();

				if (pocetZiskanychBoduPredlohy == 1)
					cv::rectangle(vyber, rohyPredlohy[0], rohyPredlohy[1], cv::Scalar(255, 255, 255));

				imshow("MAPV", vyber);
				//vyskoceni ze smycky
				if (waitKey(30) >= 0) break;

				if (pocetZiskanychBoduPredlohy == 2)
				{
					appMode = 0;
					Rect myRoi(rohyPredlohy[0], rohyPredlohy[1]);
					img_1 = vyber(myRoi);
					pocetZiskanychBoduPredlohy = 0;
					init = 1;
				}
			}



			if (init == 1)
			{
				init = 0;

				if (zmena == 1)
				{
					zmena = 0;
					if (metoda == 1)
						metoda = 0;
					else
						metoda = 1;
					appMode = 0;
				}

				//============Vypocet vyznamnych bodu pro vyhledavany obraz============
				if (metoda == 0)
				{
					detector->detect(img_1, vyzn_body_1);
					extractor->compute(img_1, vyzn_body_1, deskriptor_1);
				}
				else
				{
					orb_d->detect(img_1, vyzn_body_1);
					orb_e->compute(img_1, vyzn_body_1, deskriptor_1);
				}
				//============Vypocet pozice rohu vyhledavaneho obrazu============
				img_1_rohy[0] = cvPoint(0, 0);
				img_1_rohy[1] = cvPoint(img_1.cols, 0);
				img_1_rohy[2] = cvPoint(img_1.cols, img_1.rows);
				img_1_rohy[3] = cvPoint(0, img_1.rows);
			}
		}

	}
	return 0;
}