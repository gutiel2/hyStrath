/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2016-2021 hyStrath
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of hyStrath, a derivative work of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#include "basic2ChemistryModel.H"
#include "basic2Thermo.H"

// * * * * * * * * * * * * * * * * Selectors * * * * * * * * * * * * * * * * //
template<class ChemistryModel>
Foam::autoPtr<ChemistryModel> Foam::basic2ChemistryModel::New
(
    const fvMesh& mesh
)
{
    IOdictionary chemistryDict
    (
        IOobject
        (
            "chemistryProperties",
            mesh.time().constant(),
            mesh,
            IOobject::MUST_READ,
            IOobject::NO_WRITE,
            false
        )
    );

    word chemistryTypeName;

    if (chemistryDict.isDict("chemistryType"))
    {
        const dictionary& chemistryTypeDict
        (
            chemistryDict.subDict("chemistryType")
        );

        Info<< "Selecting chemistry type " << chemistryTypeDict << endl;

        const int nCmpt = 7;
        const char* cmptNames[nCmpt] =
        {
            "chemistrySolver",
            "chemistryThermo",
            "transport",
            "thermo",
            "equationOfState",
            "specie",
            "energy"
        };

        IOdictionary thermoDict
        (
            IOobject
            (
                "thermophysicalProperties",
                mesh.time().constant(),
                mesh,
                IOobject::MUST_READ_IF_MODIFIED,
                IOobject::NO_WRITE,
                false
            )
        );

        word thermoTypeName;

        if (thermoDict.isDict("thermoType"))
        {
            const dictionary& thermoTypeDict(thermoDict.subDict("thermoType"));
            thermoTypeName =
                word(thermoTypeDict.lookup("transport")) + '<'
              + word(thermoTypeDict.lookup("thermo")) + '<'
              + word(thermoTypeDict.lookup("equationOfState")) + '<'
              + word(thermoTypeDict.lookup("specie")) + ">>,"
              + word(thermoTypeDict.lookup("energy")) + ">";
        }
        else
        {
            FatalIOErrorIn
            (
                (ChemistryModel::typeName + "::New(const mesh&)").c_str(),
                thermoDict
            )   << "thermoType is in the old format and must be upgraded"
                << exit(FatalIOError);
        }

        // Construct the name of the chemistry type from the components
        chemistryTypeName =
            word(chemistryTypeDict.lookup("chemistrySolver")) + '<'
          + word(chemistryTypeDict.lookup("chemistryThermo")) + ','
          + thermoTypeName + ">";

        /* Old Pointer 
           typename ChemistryModel::fvMeshConstructorTable::iterator cstrIter =
               ChemistryModel::fvMeshConstructorTablePtr_->find(chemistryTypeName);
           …
        */

        // Mateo (updated for 2406)
        // 1) Grab the entire selection table by dereferencing the ptr:
        const auto& meshTable = *ChemistryModel::fvMeshConstructorTablePtr_;

        // 2) Look up our requested key:
        auto cstrIter = meshTable.find(chemistryTypeName);

        if (cstrIter == meshTable.end())
        {
            FatalErrorIn(ChemistryModel::typeName + "::New(const mesh&)")
                << "Unknown " << ChemistryModel::typeName
                << " type " << chemistryTypeName << nl << nl
                << "Valid " << ChemistryModel::typeName
                << " types are:" << nl
                << meshTable.sortedToc() << nl
                << exit(FatalError);
        }

        // 3) Invoke the constructor function we found:
        return Foam::autoPtr<ChemistryModel>( (cstrIter())(mesh) );
    }
    else
    {
        // old‐format branch
        chemistryTypeName = word(chemistryDict.lookup("chemistryType"));

        Info<< "Selecting chemistry type " << chemistryTypeName << endl;

        const auto& meshTable = *ChemistryModel::fvMeshConstructorTablePtr_;
        auto cstrIter = meshTable.find(chemistryTypeName);

        if (cstrIter == meshTable.end())
        {
            FatalErrorIn(ChemistryModel::typeName + "::New(const mesh&)")
                << "Unknown " << ChemistryModel::typeName
                << " type " << chemistryTypeName << nl << nl
                << "Valid " << ChemistryModel::typeName
                << " types are:" << nl
                << meshTable.sortedToc() << nl
                << exit(FatalError);
        }

        return Foam::autoPtr<ChemistryModel>( (cstrIter())(mesh) );
    }
}
// ************************************************************************* //