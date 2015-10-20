## Copyright (C) 2011, 2015  International Business Machines Corporation
## All Rights Reserved

package OutputAssignmentGenerator;

use strict;
use warnings;
use SPL::CodeGen;

# This function generates output attribute setters for all attributes
# of an output port that have explicit assignments

sub generate($) {

    # this function requires the name of the output tuple and the output port object as its arguments
    my ($tupleName, $outputPort) = @_;
    return unless $tupleName && $outputPort;

    # step through all of the output attributes, generating an attribute setter for each attribute that has an assignment
    for (my $i = 0; $i < $outputPort->getNumberOfAttributes(); ++$i) {

        # skip attributes that don't have assignments
        my $attribute = $outputPort->getAttributeAt($i);
        next unless $attribute->hasAssignment();

        # generate an approperiate setter for attributes that do have assignments
        my $attributeName = $attribute->getName();
        my $attributeCppType = $attribute->getCppType();
        if ($attribute->getAssignmentValue()) {
            my $cppExpression = $attribute->getAssignmentValue()->getCppExpression();
            my $splExpression = $attribute->getAssignmentValue()->getSPLExpression();
            print "\n$tupleName.set_$attributeName( $cppExpression ); // output assignment is expression '$splExpression'\n";
        } elsif ($attribute->hasAssignmentWithOutputFunction()) {
            my $functionName = $attribute->getAssignmentOutputFunctionName();
            print "\n$tupleName.set_$attributeName( $functionName() ); // output assignment is function '$functionName()'\n";
        } else {
            print "\n$tupleName.set_$attributeName( ??? ); // output assignment is ???\n";
        }
    }
    print "\n";
}

1;
