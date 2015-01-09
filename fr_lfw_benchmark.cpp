/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                           License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2014, Itseez Inc, all rights reserved.
// Third Recty copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of the copyright holders may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a Recticular purpose are disclaimed.
// In no event shall the Itseez Inc or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

#include "opencv2/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/datasets/fr_lfw.hpp"

#include "TextureFeature.h"
#include "Preprocessor.h"

#if 0
 #include "../Profile.h"
#else
 #define PROFILE ;
 #define PROFILEX(s) ;
#endif

#include <iostream>
#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include <set>

using namespace std;
using namespace cv;
using namespace cv::datasets;
//using namespace cv::face;

map<string, int> people;

int getLabel(const string &imagePath)
{
    size_t pos = imagePath.find('/');
    string curr = imagePath.substr(0, pos);
    map<string, int>::iterator it = people.find(curr);
    if (people.end() == it)
    {
        people.insert(make_pair(curr, (int)people.size()));
        it = people.find(curr);
    }
    return (*it).second;
}

void printOptions()
{
    cerr << "[extractors]  :"<< endl;
    for (size_t i=0; i<TextureFeature::EXT_MAX; ++i) {  if(i%5==0) cerr << endl; cerr << format("%10s(%2d)",TextureFeature::EXS[i],i); }
    cerr << endl << endl << "[reductors] :" << endl;
    for (size_t i=0; i<TextureFeature::RED_MAX; ++i) {  if(i%5==0) cerr << endl; cerr << format("%10s(%2d)",TextureFeature::REDS[i],i); }
    cerr << endl << endl << "[classifiers] :" << endl;
    for (size_t i=0; i<TextureFeature::CL_MAX; ++i)  {  if(i%5==0) cerr << endl; cerr << format("%10s(%2d)",TextureFeature::CLS[i],i);  }
    //cerr << endl << endl <<  "[preproc] :" << endl;
    //for (size_t i=0; i<TextureFeature::PRE_MAX; ++i) {  if(i%5==0) cerr << endl; cerr << format("%10s(%2d)",TextureFeature::PPS[i],i);  }
    cerr << endl;
}



class MyFace 
{
    Ptr<TextureFeature::Extractor> ext;
    Ptr<TextureFeature::Reductor>  red;
    Ptr<TextureFeature::Verifier>  cls;
    Preprocessor pre;
    bool doFlip;

    Mat labels;
    Mat features;

public:

    MyFace(int extract=0, int redu=0, int clsfy=0, int preproc=0, int crop=0, bool flip=false)
        : pre(preproc,crop)
        , doFlip(flip)
    {
        ext = TextureFeature::createExtractor(extract);
        red = TextureFeature::createReductor(redu);
        cls = TextureFeature::createVerifier(clsfy);
    }

    virtual int addTraining(const Mat & img, int label) 
    {
        Mat feat1;
        ext->extract(pre.process(img), feat1);

        Mat fr = feat1.reshape(1,1);
        if (! red.empty())
            red->reduce(fr,fr);

        features.push_back(fr);
        labels.push_back(label);
        cerr <<fr.cols << " i_" << labels.rows << "\r";
        return labels.rows;
    }
    virtual bool train()
    {
        cerr << "." << features.cols << "     ";
        int ok = cls->train(features, labels.reshape(1,features.rows));
        cerr << ".\r";
        CV_Assert(ok);
        features.release();
        labels.release();
        return ok!=0;
    }

    virtual int same(const Mat & a, const Mat &b) const
    {
        Mat feat1, feat2;
        ext->extract(pre.process(a), feat1);
        ext->extract(pre.process(b), feat2);
        if (! red.empty())
        {
            red->reduce(feat1,feat1);
            red->reduce(feat2,feat2);
        }
        return cls->same(feat1,feat2);
    }
};


int main(int argc, const char *argv[])
{
    PROFILE;
    const char *keys =
            "{ help h usage ? |    | show this message }"
            "{ path p         |true| path to dataset (lfw2 folder) }"
            "{ ext e          |25  | extractor enum }"
            "{ red r          |1   | reductor enum }"
            "{ cls c          |6   | classifier enum }"
            "{ pre P          |0   | preprocessing }"
            "{ crop C         |80  | cut outer 80 pixels to to 90x90 }"
            "{ flip f         |0   | add a flipped image }"
            "{ train t        |dev | train method: 'dev'(pairsDevTrain.txt) or 'split'(pairs.txt) }";

    CommandLineParser parser(argc, argv, keys);
    string path(parser.get<string>("path"));
    if (parser.has("help") || path=="true")
    {
        parser.printMessage();
        printOptions();
        return -1;
    }
    int ext = parser.get<int>("ext");
    int red = parser.get<int>("red");
    int cls = parser.get<int>("cls");
    int pre = parser.get<int>("pre");
    int crp = parser.get<int>("crop");
    bool flp = parser.get<bool>("flip");
    string trainMethod(parser.get<string>("train"));
    cout << TextureFeature::EXS[ext] << " " << TextureFeature::REDS[red] << " " << TextureFeature::CLS[cls] << " " << crp << " " << flp << " " << trainMethod << '\r';

    int64 t0 = getTickCount();
    //Ptr<myface::FaceVerifier> model = createMyFaceVerifier(ext,red,cls,pre,crp,flp);
    Ptr<MyFace> model = makePtr<MyFace>(ext,red,cls,pre,crp,flp);

    // load dataset
    Ptr<FR_lfw> dataset = FR_lfw::create();
    dataset->load(path);
    unsigned int numSplits = dataset->getNumSplits();

    if (trainMethod == "dev") // train on personsDevTrain.txt
    {
        for (unsigned int i=0; i<dataset->getTrain().size(); ++i)
        {
            FR_lfwObj *example = static_cast<FR_lfwObj *>(dataset->getTrain()[i].get());

            int currNum1 = getLabel(example->image1);
            Mat img = imread(path+example->image1, IMREAD_GRAYSCALE);
            model->addTraining(img, currNum1);

            int currNum2 = getLabel(example->image2);
            img = imread(path+example->image2, IMREAD_GRAYSCALE);
            model->addTraining(img, currNum2);
        }

        {
            PROFILEX("train");
            model->train();
        }
    }


    vector<double> p;
    for (unsigned int j=0; j<numSplits; ++j)
    {
        PROFILEX("splits");
        if (trainMethod == "split") // train on the remaining 9 splits from pairs.txt
        {
            for (unsigned int j2=0; j2<numSplits; ++j2)
            {
                if (j==j2) continue;

                vector < Ptr<Object> > &curr = dataset->getTest(j2);
                for (unsigned int i=0; i<curr.size(); ++i)
                {
                    FR_lfwObj *example = static_cast<FR_lfwObj *>(curr[i].get());
                    int currNum1 = getLabel(example->image1);
                    Mat img = imread(path+example->image1, IMREAD_GRAYSCALE);
                    model->addTraining(img, currNum1);

                    int currNum2 = getLabel(example->image2);
                    img = imread(path+example->image2, IMREAD_GRAYSCALE);
                    model->addTraining(img, currNum2);
                }
            }
            {
                PROFILEX("train");
                model->train();
            }
        }

        unsigned int incorrect = 0, correct = 0;
        vector < Ptr<Object> > &curr = dataset->getTest(j);
        for (unsigned int i=0; i<curr.size(); ++i)
        {
            PROFILEX("tests");
            FR_lfwObj *example = static_cast<FR_lfwObj *>(curr[i].get());

            Mat img1 = imread(path+example->image1, IMREAD_GRAYSCALE);
            Mat img2 = imread(path+example->image2, IMREAD_GRAYSCALE);
            bool same = model->same(img1,img2)>0;
            if (same == example->same)
                correct++;
            else
                incorrect++;
        }

        double acc = double(correct)/(correct+incorrect);
        printf("%4u %5u/%-5u  %2.3f                        \r", j, correct,incorrect,acc );
        p.push_back(acc);
    }

    double mu = 0.0;
    for (vector<double>::iterator it=p.begin(); it!=p.end(); ++it)
    {
        mu += *it;
    }
    mu /= p.size();
    double sigma = 0.0;
    for (vector<double>::iterator it=p.begin(); it!=p.end(); ++it)
    {
        sigma += (*it - mu)*(*it - mu);
    }
    sigma = sqrt(sigma/p.size());
    double se = sigma/sqrt(double(p.size()));

    int64 t1 = getTickCount();
    cerr << format("%-8s",TextureFeature::EXS[ext])  << " ";
    cerr << format("%-7s",TextureFeature::REDS[red]) << " ";
    cerr << format("%-9s",TextureFeature::CLS[cls])  << " ";
    //cerr << format("%-8s",TextureFeature::PPS[pre])  << " ";
    cerr << format("%-6s",trainMethod.c_str()) << "\t";
    //cerr << format("%2d %d %-6s",crp ,flp, trainMethod.c_str()) << "\t";
    cerr << format("%9.4f %9.4f %9.4f", mu, se, ((t1-t0)/getTickFrequency())) << endl;

    return 0;
}