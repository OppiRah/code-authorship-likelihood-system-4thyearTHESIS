#ifndef KNN_MODEL_H
#define KNN_MODEL_H

#include <string>
#include <vector>
#include <map>
#include "features.h"

struct AuthorshipResult {
    std::string student;
    std::string filepath;
    double      score;
    double      scorePct;
    std::string label;
    std::string color;
    int         profileSize;
    std::vector<std::string> styleMatch;
    bool        success;
    std::string error;
};

struct PairVerdict {
    std::string      studentA;
    std::string      studentB;
    AuthorshipResult resultA;
    AuthorshipResult resultB;
    std::string      likelyOriginal;
    std::string      likelyCopier;
    bool             success;
};

struct StudentProfile {
    std::string              name;
    std::vector<FeatureResult> submissions;
    std::vector<double>      profileVector;
    std::vector<StyleNote>   styleNotes;
};

class AuthorshipScorer {
public:
    explicit AuthorshipScorer(int k = 3);
    bool             loadSubmission(const std::string& studentName,
                                    const std::string& filepath);
    AuthorshipResult score(const std::string& studentName,
                           const std::string& filepath);
    PairVerdict      scorePair(const std::string& studentA,
                               const std::string& fileA,
                               const std::string& studentB,
                               const std::string& fileB);
    std::vector<std::string> readyStudents() const;
    const StudentProfile*    getProfile(const std::string& name) const;
    int                      studentCount() const;
private:
    int k_;
    std::map<std::string, StudentProfile> profiles_;
    void rebuildProfile(StudentProfile& profile);
};

std::pair<std::string,std::string> classifyAuthorship(double score);

#endif