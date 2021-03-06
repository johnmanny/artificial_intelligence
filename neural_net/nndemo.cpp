/*
	Author: John Nemeth
        Info: back prop algorithm taken from 'artificial intelligence
                modern approach' and other various info cited in comments
*/

#include <iostream>
#include <string.h>
#include <math.h>
#include <fstream>
#include <random>
#include <limits>

#define INPUTNEURONS 9
#define HIDDENNEURONS 3
#define OUTPUTNEURONS 1
#define EXAMPLECOUNT 12

using namespace std;

////////////////////////////////////////////////////////////////////
/* structs for NN */
////////////////////////////////////////////////////////////////////

///////////////////////////////////////
// reference structure from https://www.youtube.com/watch?v=PQo78WNGiow
typedef struct Neuron {

    void activate() { this->activatedVal = (1.0 / (1.0 + exp((-this->val)) ) ); };
    void derive() {this->derivedVal = (this->activatedVal * (1.0 - this->activatedVal)); };
    double val;
    double activatedVal;
    double derivedVal;
} neuron;

///////////////////////////////////////
/* struct for examples 
	-input vector attributes ordered as: 
	alternate, bar, fri/sat, patrons, price, raining, reservation, type, waitestimate
	-output value is willwait
*/
typedef struct Example {

    double inputsByOrder[9];
    double output;
} example;

///////////////////////////////////////
// struct for neural network
typedef struct NeuralNetwork {

    int layerCount;
    /* inputWeights[0] = weight from input node 1 to hidden node 1
	inputWeights[3] = weight from input node 2 to hidden node 1
	inputWeights[17] = weight from input nod 9 to hidden node 3
    */
    double inputWeights[INPUTNEURONS * HIDDENNEURONS];
    double hiddenWeights[HIDDENNEURONS * OUTPUTNEURONS];
    neuron inputNeurons[INPUTNEURONS];
    neuron hiddenNeurons[HIDDENNEURONS];
    neuron outputNeuron;

} neuralNet;

////////////////////////////////////////////////////////////////////
/* for example inputs, Yes = 0.95, No = 0.1, 
			None = 0.2, Some = 0.3, Full = 0.4
			$ = 0.5, $$ = 0.6, $$$ = 0.7
			French = 0.72, Thai = 0.74, Burger = 0.76, Italian = 0.78
			0-10 = 0.8, 10-30 = 0.83, 30-60 = 0.86, >60 = 0.89 
*/
void initExamples(example * exampleSet) {

    ifstream input;
    input.open("examples.txt");
    if (!input) {
        cerr << "ERROR! examples.txt not found" << endl;
    }

    char string[257];
    int exampleNum = 0, attrNum = 0;
    double numToSet = 0.0;
    while (!input.eof()) {

        input.getline(string, 256, ' ');

	if ((strcmp(string, "") == 0) || (strcmp(string, "\n") == 0))
            continue;
	else if (strcmp(string, "yes") == 0)
            numToSet = 0.95;
	else if (strcmp(string, "no") == 0)
            numToSet = 0.1;
	else if (strcmp(string, "none") == 0)
            numToSet = 0.2;
	else if (strcmp(string, "some") == 0)
            numToSet = 0.3;
	else if (strcmp(string, "full") == 0)
            numToSet = 0.4;
	else if (strcmp(string, "$") == 0)
            numToSet = 0.5;
	else if (strcmp(string, "$$") == 0)
            numToSet = 0.6;
	else if (strcmp(string, "$$$") == 0)
            numToSet = 0.7;
	else if (strcmp(string, "french") == 0)
            numToSet = 0.72;
	else if (strcmp(string, "thai") == 0)
            numToSet = 0.74;
	else if (strcmp(string, "burger") == 0)
            numToSet = 0.76;
	else if (strcmp(string, "italian") == 0)
            numToSet = 0.78;
	else if (strcmp(string, "0-10") == 0)
            numToSet = 0.8;
	else if (strcmp(string, "10-30") == 0)
            numToSet = 0.83;
	else if (strcmp(string, "30-60") == 0)
            numToSet = 0.86;
	else if (strcmp(string, ">60") == 0)
            numToSet = 0.89;
        else
            cerr << "DEFAULT CHAR REACHED WITH INPUT: '" << string << "'" << endl;

        if (attrNum != 9) {
            exampleSet[exampleNum].inputsByOrder[attrNum] = numToSet;
            attrNum++;
        }
        else {
            exampleSet[exampleNum].output = numToSet;
            exampleNum++;
            attrNum = 0;
        }
    }
}

////////////////////////////////////////////////////////////////////
/* init some NN values */
void initNN(neuralNet * nn) {

    nn->outputNeuron.activatedVal = 500.0;
    nn->layerCount = 3;
}

////////////////////////////////////////////////////////////////////
/* prints NN info */
void printNN(neuralNet * nn) {

    cerr << "---INPUT BREAKDOWN:" << endl;
    char inputName[256];
    for (int i = 0; i < INPUTNEURONS; i++) {
        switch (i) {
            case 0:
                strcpy(inputName, "alt");
                break;
            case 1:
                strcpy(inputName, "bar");
                break;
            case 2:
                strcpy(inputName, "fri/sat");
                break;
            case 3:
                strcpy(inputName, "patrons");
                break;
            case 4:
                strcpy(inputName, "price");
                break;
            case 5:
                strcpy(inputName, "raining");
                break;
            case 6:
                strcpy(inputName, "reserv.");
                break;
            case 7:
                strcpy(inputName, "type");
                break;
            case 8:
                strcpy(inputName, "waitest");
                break;
        }
        for (int y = 0; y < HIDDENNEURONS; y++) {
            cerr << inputName << "\t->hidden" << y << " weight: " << nn->inputWeights[(i * 3) + y] << endl;
        }
        //cerr << "ending activatedVal: " << nn->inputNeurons[i].activatedVal << endl;
    }
    for (int i = 0; i < HIDDENNEURONS; i++) {
        cerr << "hidden" << i << "->output weight: " << nn->hiddenWeights[i] << endl;
        cerr << "\tval: " << nn->hiddenNeurons[i].val << " activated Val: " << nn->hiddenNeurons[i].activatedVal
             << " derived val: " << nn->hiddenNeurons[i].derivedVal << endl;
    }
    cerr << "output's val: " << nn->outputNeuron.val << " activated Val: " << nn->outputNeuron.activatedVal
         << " derived val: " << nn->outputNeuron.derivedVal << endl;
    cerr << "yes: 0.95 no: 0.1" << endl;
}

////////////////////////////////////////////////////////////////////
/* performs back prop algorithm for nn learning */
void backPropLearning(example * examplesArr, neuralNet * nn) {

    int inputWCount = INPUTNEURONS * HIDDENNEURONS;
    int exampleIterations = 0;
    // used for generating random weights
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<> dis(0.0, 1.0);

    while (1) {

        nn->outputNeuron.activatedVal = 500.0;		// reset activated val for convergence tests
        // assign random initial weights for each neuron connection
        for (int i = 0; i < inputWCount; i++) {
            nn->inputWeights[i] = dis(gen);
        }
        for (int i = 0; i < HIDDENNEURONS; i++) {
            nn->hiddenWeights[i] = dis(gen);
        }

        int j;
        double outputError = 1.0, oldError, oldActivated;
        // loop through examples 
        for (j = 0; j < EXAMPLECOUNT; j++) {
            /* ---propogate inputs forward to output---*/
            for (int v = 0; v < INPUTNEURONS; v++) {	// assign initial vals
                nn->inputNeurons[v].val = examplesArr[j].inputsByOrder[v];
                nn->inputNeurons[v].activatedVal = examplesArr[j].inputsByOrder[v];
            }
            for (int x = 0; x < HIDDENNEURONS; x++) {		// produce vals for hidden neurons
                nn->hiddenNeurons[x].val = 0.5;		// init as .4 for bias
                for (int z = 0; z < INPUTNEURONS; z++) {	// sum vals of each input and weight
                    nn->hiddenNeurons[x].val += (nn->inputNeurons[z].activatedVal * nn->inputWeights[x + (3 * z)]);
                }
                nn->hiddenNeurons[x].activate();	// executes activation func tied to neurons
            }

            nn->outputNeuron.val = 0.4;		// for bias
            for (int x = 0; x < HIDDENNEURONS; x++) {	// propagate input to output neuron
                nn->outputNeuron.val += (nn->hiddenNeurons[x].activatedVal * nn->hiddenWeights[x]);
            }
            oldActivated = nn->outputNeuron.activatedVal;
            nn->outputNeuron.activate();

            /*---propogate error backwards---*/
            // find error of output
            nn->outputNeuron.derive();
            oldError = outputError;
            outputError = (nn->outputNeuron.derivedVal * (examplesArr[j].output - nn->outputNeuron.activatedVal));
            // find error of hidden neurons
            double hiddenError[HIDDENNEURONS];
            for (int x = 0; x < HIDDENNEURONS; x++) {
                nn->hiddenNeurons[x].derive();
                hiddenError[x] = nn->hiddenNeurons[x].derivedVal *
                                 (nn->hiddenWeights[x] * outputError);
            }

            int neuWeightStartIndex;
            /* ---update each weight using errors--- */
            // update input weights
            for (int x = 0; x < INPUTNEURONS; x++) {
                neuWeightStartIndex = x * 3;
                for (int y = 0; y < HIDDENNEURONS; y++) {
                    nn->inputWeights[neuWeightStartIndex + y] += (0.084 * nn->inputNeurons[x].activatedVal * hiddenError[y]);
                }
            }
            // update hidden neuron weights
            for (int x = 0; x < HIDDENNEURONS; x++) {
                nn->hiddenWeights[x] += (0.084 * nn->hiddenNeurons[x].activatedVal * outputError);
            }

        }
        // test for convergence
        if ((outputError < 0.00001) && (outputError > 0.0)) {

            cout.precision(15);
            cout << "\tPASSED OUTPUTERROR TEST - VALUE: " << fixed << outputError << endl;

            // variables to test for final example evaluation (convergence)
            double actDiff = fabs(oldActivated - nn->outputNeuron.activatedVal);
            double errorDiff = fabs(oldError - outputError);
            if ((actDiff > 0.01) && (errorDiff > 0.01)) {	// difference arbitrarily unacceptable
                cout << "\tFAILED CONVERGENCE TEST - CONTINUING BACK PROP LEARNING..." << endl;
                exampleIterations = 0;
                continue;
            }
            cout << "PASSED CONVERGENCE TEST\noutput difference upon final example: " << actDiff
                 << "\nerror difference upon final example: " << errorDiff << endl;
            cout << "loops through example set: " << exampleIterations << endl << endl;
            printNN(nn);
            return;
        }
        exampleIterations++;
    }
}

////////////////////////////////////////////////////////////////////
int main() {

    // initialize examples
    example exampleSet[12];
    neuralNet backPropNN;

    initExamples(exampleSet);

    // initialize neuralnetwork
    initNN(&backPropNN);
    backPropLearning(exampleSet, &backPropNN);

    return 0;
}
