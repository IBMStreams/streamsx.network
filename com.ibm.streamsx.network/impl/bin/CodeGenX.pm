## Copyright (C) 2011, 2015  International Business Machines Corporation
## All Rights Reserved

package CodeGenX;

use strict;
use warnings;
use SPL::CodeGen;

# This function is similar to SPL::CodeGen::getOutputTupleCppAssignments() in
# that it generates calls to setter functions for attributes of an output port
# that have explicit assignments (that is the case when
# "$attribute->hasAssignment()" is true) in the 'output" clause of the operator
# declaration. It handles the additional case of values that are
# operator-specific attribute assignment functions (that is the case where
# "$attribute->hasAssignmentWithOutputFunction()" is true and
# "$attribute->getAssignmentOutputFunctionName()" is non-null), as well as the
# case of values that are SPL expressions (that is the case where
# "$attribute->getAssignmentValue()" is non-null).

sub assignOutputAttributeValues($$) {

    # this function requires the name of the output tuple and the output port object 
    my ($tupleName, $outputPort) = @_;
    return unless $tupleName && $outputPort;

    # step through all of the output attributes, generating attribute setters 
    # for those that are explicitly assigned values in the 'output' clause
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
            print "\n$tupleName.set_$attributeName( $cppExpression ); // value is SPL expression '$splExpression'\n";
        } elsif ($attribute->hasAssignmentWithOutputFunction()) {
            my $functionName = $attribute->getAssignmentOutputFunctionName();
            print "\n$tupleName.set_$attributeName( $functionName() ); // value is operator's output attribute assignment function '$functionName()'\n";
        } else {
            print "\n$tupleName.set_$attributeName( ??? ); // value is ???\n";
        }
    }
    print "\n";
}


# This function generates setters for output attributes that do not have
# explicit assignments in the "output" clause of the operator, but do match an
# input attribute in name and type. The value of the matching input attribute is
# copied to the output attribute.

sub copyOutputAttributesFromInputAttributes($$$) {

    my ($tupleName, $outputPort, $inputPort) = @_;
    return unless $tupleName && $outputPort && $inputPort;

    my $inputPortIndex = $inputPort->getIndex();

    # step through all of the output attributes, generating attribute setters 
    # for those that are not explicitly assigned values in the 'output' clause,
    # but do match an input attribute in name and type
    for (my $i = 0; $i < $outputPort->getNumberOfAttributes(); $i++) {

        # skip attributes that have explicit assignments
        my $outAttribute = $outputPort->getAttributeAt($i);
        next if $outAttribute->hasAssignment();

        # get the name and type of this output attribute
        my $outName = $outAttribute->getName();
        my $outType = $outAttribute->getSPLType();

        # look through the input attributes for one that matches this output
        # attribute in name and type, and generate a setter if found
        for (my $j = 0; $j < $inputPort->getNumberOfAttributes(); $j++) {
            my $inName = $inputPort->getAttributeAt($j)->getName();
            my $inType = $inputPort->getAttributeAt($j)->getSPLType();
            if ($outName eq $inName && $outType eq $inType) {
                print "\n$tupleName.set_$outName(iport\$$inputPortIndex.get_$inName()); //copy value from input tuple";
                last;
            }
        }
    }
    print "\n";
}


1;
