## Copyright (C) 2015  International Business Machines Corporation
## All Rights Reserved

package IPASNEnricherCommon;

use strict;
use File::Basename;

sub init($) {
	my ($model) = @_;
	
	unshift @INC, dirname ($model->getContext()->getOperatorDirectory()) . "/../impl/nl/include";
	require NetworkResources;
	
	## inputPort0
    $::inputPort0 = $model->getInputPortAt(0);
    $::inputPort0CppType = $::inputPort0->getCppTupleType();
    $::inputPort0CppName = $::inputPort0->getCppTupleName();
	$::inputPort0SPLType = $::inputPort0->getSPLTupleType();
	
	## inputPort1
    $::inputPort1 = $model->getInputPortAt(1);
    if(defined $::inputPort1) {
	    $::inputPort1CppType = $::inputPort1->getCppTupleType();
	    $::inputPort1CppName = $::inputPort1->getCppTupleName();
		$::inputPort1SPLType = $::inputPort1->getSPLTupleType();
		
		$::filenameAttribute = $::inputPort1->getAttributeAt(0)->getName();	    	
    }

    ## outputPort0
    $::outputPort0 = $model->getOutputPortAt(0);
    $::outputPort0CppType = $::outputPort0->getCppTupleType();
    $::outputPort0CppName = $::outputPort0->getCppTupleName();
    $::outputPort0SPLType = $::outputPort0->getSPLTupleType();
		
    ## inputIPAttrParam
    $::inputIPAttrParam = $model->getParameterByName("inputIPAttr");
 	$::inputIPAttrParamCppValue = $::inputIPAttrParam->getValueAt(0)->getCppExpression();
    $::inputIPAttrParamCppType = $::inputIPAttrParam->getValueAt(0)->getCppType();
    $::inputIPAttrParamSPLValue = $::inputIPAttrParam->getValueAt(0)->getSPLExpression();
    $::inputIPAttrParamSPLType = $::inputIPAttrParam->getValueAt(0)->getSPLType();
	
	$::isScalar = isScalar($::inputIPAttrParamSPLType);
	$::isVector = isVector($::inputIPAttrParamSPLType);
	$::isMatrix = isMatrix($::inputIPAttrParamSPLType);
	
#	SPL::CodeGen::warnln($::inputIPAttrParamSPLType);
#	SPL::CodeGen::warnln("isScalar=" . $::isScalar . ", isVector=" . $::isVector . ", isMatrix=" . $::isMatrix);
	
	## check that output attribute types match input attribute types
		
	foreach my $attribute (@{$::outputPort0->getAttributes()}) {
		my $name = $attribute->getName();
		my $operation = $attribute->getAssignmentOutputFunctionName();
		if($operation eq "getASNInfo" || $operation eq "getASNumber") {
			$::outputAttributeCppType = $attribute->getCppType();
			$::outputAttributeSPLType = $attribute->getSPLType();
			
			if($::isScalar && !isScalar($::outputAttributeSPLType))
			{
				SPL::CodeGen::exitln(NetworkResources::NETWORK_INVALID_OUTPUT_ATTRIBUTE_TYPE($name, "scalar"));
			}
			
			if($::isVector && !isVector($::outputAttributeSPLType))
			{
				SPL::CodeGen::exitln(NetworkResources::NETWORK_INVALID_OUTPUT_ATTRIBUTE_TYPE($name, "vector"));
			}
			
			if($::isMatrix && !isMatrix($::outputAttributeSPLType))
			{
				SPL::CodeGen::exitln(NetworkResources::NETWORK_INVALID_OUTPUT_ATTRIBUTE_TYPE($name, "matrix"));
			}
		}
	}
	
	if($::isScalar) {
		$::baseCppOutputType = $::outputAttributeCppType;
	} elsif($::isVector) {
		$::baseCppOutputType = $::outputAttributeCppType . "::value_type" ;
	} elsif($::isMatrix) {
		$::baseCppOutputType = $::outputAttributeCppType . "::value_type::value_type";
	}
	
	## blocksIPv4FileParam
    $::asnIPv4FileParam = $model->getParameterByName("asnIPv4File");
 	if (defined $::asnIPv4FileParam) {
    	$::asnIPv4FileParamCppValue = $::asnIPv4FileParam->getValueAt(0)->getCppExpression();
        $::asnIPv4FileParamCppType = $::asnIPv4FileParam->getValueAt(0)->getCppType();
        $::asnIPv4FileParamSPLValue = $::asnIPv4FileParam->getValueAt(0)->getSPLExpression();
        $::asnIPv4FileParamSPLType = $::asnIPv4FileParam->getValueAt(0)->getSPLType();
    }

	## asnIPv6FileParam
    $::asnIPv6FileParam = $model->getParameterByName("asnIPv6File");
 	if (defined $::asnIPv6FileParam) {
    	$::asnIPv6FileParamCppValue = $::asnIPv6FileParam->getValueAt(0)->getCppExpression();
        $::asnIPv6FileParamCppType = $::asnIPv6FileParam->getValueAt(0)->getCppType();
        $::asnIPv6FileParamSPLValue = $::asnIPv6FileParam->getValueAt(0)->getSPLExpression();
        $::asnIPv6FileParamSPLType = $::asnIPv6FileParam->getValueAt(0)->getSPLType();
    }
}
1;

sub validate($) {
	my ($model) = @_;

	unshift @INC, dirname ($model->getContext()->getOperatorDirectory()) . "/../impl/nl/include";
	require NetworkResources;

	## validate that the inputIPAttr parameter contains an attribute with one of the following types:
	## - rstring
	## - list<rstring>
	## - list<list<rstring>>
	if($::isScalar && !SPL::CodeGen::Type::isRString($::inputIPAttrParamSPLType))
	{
		SPL::CodeGen::exitln(NetworkResources::NETWORK_ATTRIBUTE_PARAMETER_HAS_WRONG_TYPE("inputIPAttr", "'rstring', 'list<rstring>' or 'list<list<rstring>>'", $::inputIPAttrParamSPLType));
	}
	
	if($::isVector)
	{
		my $elemType = SPL::CodeGen::Type::getElementType($::inputIPAttrParamSPLType);
		if(!SPL::CodeGen::Type::isRString($elemType))
		{
			SPL::CodeGen::exitln(NetworkResources::NETWORK_ATTRIBUTE_PARAMETER_HAS_WRONG_TYPE("inputIPAttr", "'rstring', 'list<rstring>' or 'list<list<rstring>>'", $::inputIPAttrParamSPLType));
		}
	}
	
	if($::isMatrix)
	{
		my $elemType = SPL::CodeGen::Type::getElementType($::inputIPAttrParamSPLType);
		$elemType = SPL::CodeGen::Type::getElementType($elemType);
		if(!SPL::CodeGen::Type::isRString($elemType))
		{
			SPL::CodeGen::exitln(NetworkResources::NETWORK_ATTRIBUTE_PARAMETER_HAS_WRONG_TYPE("inputIPAttr", "'rstring', 'list<rstring>' or 'list<list<rstring>>'", $::inputIPAttrParamSPLType));
		}
	}

	if(defined $::inputPort1) {
		if(defined $::asnIPv6FileParam)
		{
			SPL::CodeGen::exitln(NetworkResources::NETWORK_PARAMETER_NOT_ALLOWED_WHEN_INPUT_PORT_1_IS_DEFINED("asnIPv6File"));
		}		
		if(defined $::asnIPv4FileParam)
		{
			SPL::CodeGen::exitln(NetworkResources::NETWORK_PARAMETER_NOT_ALLOWED_WHEN_INPUT_PORT_1_IS_DEFINED("asnIPv4File"));
		}
	}
	
	if(!defined $::inputPort1 && !defined $::asnIPv4FileParam && !defined $::asnIPv6FileParam) {
		SPL::CodeGen::exitln(NetworkResources::NETWORK_EITHER_PARAMETER_A_OR_B_MUST_BE_SPECIFIED("asnIPv4File", "asnIPv6File"));
	}
}
1;

sub isScalar($) {
	my ($elemType) = @_;
	
	if(SPL::CodeGen::Type::isTuple($elemType))
	{
		$elemType = (SPL::CodeGen::Type::getAttributeTypes($elemType))[0];
	}
	
	return !SPL::CodeGen::Type::isList($elemType); 
}
1;

sub isVector($) {
	my ($elemType) = @_;
	if(SPL::CodeGen::Type::isTuple($elemType))
	{
		$elemType = (SPL::CodeGen::Type::getAttributeTypes($elemType))[0];
	}
	
	if(SPL::CodeGen::Type::isList($elemType)) {
		# element type should NOT be another list (otherwise it is a matrix)
		$elemType = SPL::CodeGen::Type::getElementType($elemType);
		return !SPL::CodeGen::Type::isList($elemType);
	}
	
	return 0;
}
1;

sub isMatrix($) {
	my ($elemType) = @_;
	if(SPL::CodeGen::Type::isTuple($elemType))
	{
		$elemType = (SPL::CodeGen::Type::getAttributeTypes($elemType))[0];
	}
	
	if(SPL::CodeGen::Type::isList($elemType)) {
		# element type should be another list
		my $elemType = SPL::CodeGen::Type::getElementType($elemType);
		return SPL::CodeGen::Type::isList($elemType); 
	}
	
	return 0;
}
1;