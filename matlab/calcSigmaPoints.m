function [sigmaPoints] = calcSigmaPoints(covarianceMatrix,scalingParam,state)
    sigmaPoints=(state);
    stateVector =  objecttoVector(state);
    squareRoot = sqrtm(size(covarianceMatrix,1)*scalingParam*covarianceMatrix);
    squareRoot
    for row=squareRoot
        row
        sigmaPoints = [sigmaPoints, vectortoObject(stateVector + row.',state)]
        sigmaPoints = [sigmaPoints, vectortoObject(stateVector - row.',state)]
    end
    sigmaPoints
end

