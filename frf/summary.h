#ifndef SUMMARY_H
#define SUMMARY_H

// Copyright 2014 Tom SF Haines

// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at

//   http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.



// Given a data set and index view this can summarise the statistics of the nodes within - it is this that provides the output from a random forest. Internal object only though - it converts the output to a list for external consumption...

#include <Python.h>
#include <structmember.h>

typedef struct DataMatrix DataMatrix;
typedef struct IndexView IndexView;



// Summary type, just a void pointer used for all of the summary types...
typedef void * Summary;



// The function pointer typedefs required by each summary object...

// Creates a new Summary object of the given type - requires a DataMatrix to summarise, an exemplar index view to tell it which exemplars to summarise and a feature index of which index to summarise...
typedef Summary (*SummaryNew)(DataMatrix * dm, IndexView * view, int feature);

// Standard delete...
typedef void (*SummaryDelete)(Summary this);

// Calculates the error of the given nodes reaching this summary, as some kind of floating point value summed for all entries...
typedef float (*SummaryError)(Summary this, DataMatrix * dm, IndexView * view, int feature);

// Converts a set of summaries (trees is the number) into a python object that the user can dance with; returns a new reference. Can in principal return NULL and raise an error. For combining the summaries from the leaves of multiple trees into a single entity for a user to play with. offset is normally set to 0 and a little strange - its the number of bytes to add to each pointer to get the true pointer to the summary object, so Summary = *(bytes(sums[tree]) + offset) - allows for some fun optimisation when used within a SummarySet...
typedef PyObject * (*SummaryMergePy)(int trees, Summary * sums, int offset);

// As above, but for multiple test exemplars, point being the Python summary can give a datamatrix-like response and be much more efficient this way. Outer is exemplars, inner is trees when going through the sums array...
typedef PyObject * (*SummaryMergeManyPy)(int exemplars, int trees, Summary * sums, int offset);

// Load from a pointer to a memory block, optionally outputs how many bytes its read. On error will return null and set a python error...
typedef Summary (*SummaryFromBytes)(void * in, size_t * ate);

// For converting the object into a blob of bytes - first returns how many bytes are required, second outputs them...
typedef size_t (*SummarySize)(Summary this);
typedef void (*SummaryToBytes)(Summary this, void * out);


// The summary type - basically all the function pointers and documentation required to run a summary object...
typedef struct SummaryType SummaryType;

struct SummaryType
{
 const char code; // Used for specifying the summary types as a string.
 const char * name;
 const char * description;
 
 SummaryNew init;
 SummaryDelete deinit;
 
 SummaryError error;
 
 SummaryMergePy merge_py;
 SummaryMergeManyPy merge_many_py;
 
 SummaryFromBytes from_bytes;
 SummarySize size;
 SummaryToBytes to_bytes;
};



// Define a set of standard methods for arbitrary Summary objects - all assume the first entry in the Summary structure is a pointer to its SummaryType object - match with defined function pointers...
Summary Summary_new(char code, DataMatrix * dm, IndexView * view, int feature);
void Summary_delete(Summary this);

const SummaryType * Summary_type(Summary this);
float Summary_error(Summary this, DataMatrix * dm, IndexView * view, int feature);

PyObject * Summary_merge_py(int trees, Summary * sums, int offset);
PyObject * Summary_merge_many_py(int exemplars, int trees, Summary * sums, int offset);

Summary Summary_from_bytes(void * in, size_t * ate);
size_t Summary_size(Summary this);
void Summary_to_bytes(Summary this, void * out);



// The SummaryType objects provided by the system...
// Does nothing - mostly useful if using something like the BiGaussianType...
const SummaryType NothingSummary; // Code = N

// Default for dealing with discrete variables...
const SummaryType CategoricalSummary; // Code = C

// Default for dealing with continuous variables...
const SummaryType GaussianSummary; // Code = G

// Does a bivariate Gaussian, on the provided feature index and the following one (A type code vector with this as the last entry will cause a crash.)...
const SummaryType BiGaussianSummary; // Code = B



// List of all summary types known to the system - for automatic detection...
extern const SummaryType * ListSummary[];



// Because we deal in mulivariate output we need summaries to come as a set, indexed by output feature...
typedef struct SummarySet SummarySet;

struct SummarySet
{
 int features;
 Summary feature[0];
};



// Creates a SummarySet, using the type string - if the type string is null then it uses the default, where it uses a Categorical for discrete data and a Gaussian for continuous data. It also falls back to these when the string is too short...
SummarySet * SummarySet_new(DataMatrix * dm, IndexView * view, const char * codes);

// Delete...
void SummarySet_delete(SummarySet * this);

// Outputs the error of the summary set when applied to the given exemplars - used for calculating the OOB error - outputs a value for each feature, into an array of floats (length must be number of features), so the user can decide what they care about and weight them accordingly. It adds its value to whatever is already in the array...
void SummarySet_error(SummarySet * this, DataMatrix * dm, IndexView * view, float * out);

// Returns a new reference to a Python object that is returned to the user to summarise the many summary sets provided - a tuple indexed by feature, with the actual objects in the tuple defined by the actual summary types. This exists to be given the summary sets at the leaves of a forest that an exemplar falls into...
PyObject * SummarySet_merge_py(int trees, SummarySet ** sum_sets);

// As above, but for when we are processing an entire data matrix and hence have an exemplars x trees array of SummarySet pointers, indexed with exemplars in the outer loop, trees in the inner...
PyObject * SummarySet_merge_many_py(int exemplars, int trees, SummarySet ** sum_sets);

// Given a buffer of bytes saved from a summary set this loads it and returns a new summary set; cna optionally provide a pointer to have the number of bytes read written into. Will return null and set a python error if there is a problem...
SummarySet * SummarySet_from_bytes(void * in, size_t * ate);

// Converts a summary set into bytes - first gives how many, second does the deed...
size_t SummarySet_size(SummarySet * this);
void SummarySet_to_bytes(SummarySet * this, void * out);



#endif