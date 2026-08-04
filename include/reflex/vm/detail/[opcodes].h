
//CORE (cant be handled by ExternalFunctions)

OP(PushConst)				//flags for size

OP(PushGlobal)				//copy a global value @ location ontop of stack (param32 = {location, size{), note: objects are pushed as adr, so no PushScopeObject opcode needed (as no retain/release)
OP(PushGlobalByAdr)

OP(AssignGlobalValue)		//write the global value @ location with top of stack value and pop
OP(AssignGlobalObject)

OP(PushLocal)
OP(PushLocalByAdr)

OP(AssignLocalValue)
OP(AssignLocalObject)

OP(PushMember)	//flags for size
OP(PushMemberByAdr)	//flags for size

OP(AssignMemberValue)
OP(AssignMemberObject)

OP(Discard)

OP(SwizzleTemporaryValue)	//selects a value sub-member of struct on top of stack.  only needed for temporaries, variables are done in place


OP(Marker)	//used in linking stage, replace with DATA, dont need dedicate opcode
OP(Jump)

OP(JumpIfFalse8)
OP(JumpIfTrue8)
OP(JumpIfFalse32)
OP(JumpIfTrue32)

OP(Switch8)
OP(Switch32)

OP(BindFnObject)

OP(CallFn)	//store current pointer before jump		flags = Tuple<UInt8,UInt8,UInt8,UInt8>(offset, size, 0, inlined flag) (inlined used by linker)
OP(CallFnObject)
OP(Return)				//param32 = size
OP(ReturnObject)				//param32 = size

OP(CallExternal)

OP(Data)	//additional data



//INTRINSICS (can be used client side instead of ExternalFunctions)

OP(intrinsicNullObject)	//create object of type and write pointer to stack
OP(intrinsicNewObject)	//create object of type and write pointer to stack
OP(intrinsicObjectCast)

OP(intrinsicValueEqual)
OP(intrinsicValueInequal)

OP(intrinsicLogicalNot8)
OP(intrinsicLogicalNot32)

OP(intrinsicLogicalOr8)
OP(intrinsicLogicalOr32)

OP(intrinsicLogicalAnd8)
OP(intrinsicLogicalAnd32)

OP(intrinsicInt32LessThan)
//OP(intrinsicInt32LessThanOrEqual)		//TODO
OP(intrinsicInt32GreaterThan)
//OP(intrinsicInt32GreaterThanOrEqual)	//TODO

OP(intrinsicPreIncInt32)
OP(intrinsicPreDecInt32)
OP(intrinsicPostIncInt32)
OP(intrinsicPostDecInt32)

OP(intrinsicAddAssignInt32)
OP(intrinsicSubtractAssignInt32)
OP(intrinsicMulAssignInt32)

OP(intrinsicInvertInt32)

OP(intrinsicAddInt32Pair)
OP(intrinsicSubInt32Pair)
OP(intrinsicMulInt32Pair)
OP(intrinsicDivInt32Pair)

OP(intrinsicFloat32LessThan)
OP(intrinsicFloat32GreaterThan)

OP(intrinsicAddAssignFloat32)
OP(intrinsicSubtractAssignFloat32)
OP(intrinsicMulAssignFloat32)

OP(intrinsicInt32ToFloat32)
OP(intrinsicInvertFloat32)

OP(intrinsicAddFloat32Pair)
OP(intrinsicSubFloat32Pair)
OP(intrinsicMulFloat32Pair)
OP(intrinsicDivFloat32Pair)

OP(intrinsicInt32Next)
OP(intrinsicIntegralArrayNext)	//param = stride
OP(intrinsicObjectArrayNext)

OP(intrinsicValueArrayGet)		//param = stride
OP(intrinsicValueArraySet)		//param = stride
OP(intrinsicValueArray32Get)
OP(intrinsicValueArray32Set)

OP(intrinsicStringToBool)
OP(intrinsicStringToKey32)
OP(intrinsicStringEqual)
OP(intrinsicStringInequal)

OP(intrinsicObjectMethod_Void)
OP(intrinsicObjectMethod_Object)
OP(intrinsicObjectMethod_Value8)
OP(intrinsicObjectMethod_Value32)
OP(intrinsicObjectMethod_Float32)

OP(intrinsicObjectMethod_Void_Object)
OP(intrinsicObjectMethod_Void_Value8)
OP(intrinsicObjectMethod_Void_Value32)
OP(intrinsicObjectMethod_Void_Float32)

OP(intrinsicObjectMethod_Void_Value32_Object)
OP(intrinsicObjectMethod_Value8_Object)
OP(intrinsicObjectMethod_Value8_Value32)
OP(intrinsicObjectMethod_Value32_Object)
OP(intrinsicObjectMethod_Float32_Value32)
OP(intrinsicObjectMethod_Object_Value32)

OP(intrinsicObjectMethod_Void_Value32_Value32)
OP(intrinsicObjectMethod_Void_Float32_Float32)

OP(intrinsicObjectMethod_Object_Value32_Value32)
OP(intrinsicObjectMethod_Value8_Object_Object)

//OPTIMISATIONS other opcodes are converted to these during optimisation stage

OP(optimisationPushConst8)
OP(optimisationPushConst32)
OP(optimisationPushConst64)

OP(optimisationPushGlobal8)
OP(optimisationPushGlobal32)
OP(optimisationPushGlobal64)
OP(optimisationPushGlobal32Pair)
OP(optimisationPushGlobal64Pair)
OP(optimisationPushGlobal64and32)
OP(optimisationPushGlobal32andConst32)
OP(optimisationPushGlobal64andConst32)

OP(optimisationPushLocal8)
OP(optimisationPushLocal32)
OP(optimisationPushLocal64)
OP(optimisationPushLocal32Pair)
OP(optimisationPushLocal64Pair)
OP(optimisationPushLocal64and32)
OP(optimisationPushLocal32andConst32)
OP(optimisationPushLocal64andConst32)

OP(optimisationPushMember8)
OP(optimisationPushMember32)
OP(optimisationPushMember64)

OP(optimisationAssignGlobalValue8)
OP(optimisationAssignGlobalValue32)
OP(optimisationAssignGlobalValue64)
OP(optimisationAssignGlobalObjectST)

OP(optimisationAssignLocalValue8)
OP(optimisationAssignLocalValue32)
OP(optimisationAssignLocalValue64)
OP(optimisationAssignLocalObjectST)

OP(optimisationAssignMemberValue8)
OP(optimisationAssignMemberValue32)
OP(optimisationAssignMemberValue64)
OP(optimisationAssignMemberObjectST)

OP(optimisationAssignPushGlobalObject)
OP(optimisationAssignPushGlobalObjectST)
OP(optimisationAssignPushLocalObject)
OP(optimisationAssignPushLocalObjectST)

OP(optimisationDiscard8)
OP(optimisationDiscard32)
OP(optimisationDiscard64)

OP(optimisationSwitch8JumpTable)

OP(optimisationValueEqual32)
OP(optimisationValueInequal32)
OP(optimisationValueEqual64)
OP(optimisationValueInequal64)

OP(optimisationCallFnOfValues)	//no objects
OP(optimisationReturnVoid)
OP(optimisationReturn8)			//currently must not include structs, due to lack of seperate ShuffleMember opcode
OP(optimisationReturn32)		//currently must not include structs, due to lack of seperate ShuffleMember opcode
OP(optimisationReturn64)		//currently must not include structs, due to lack of seperate ShuffleMember opcode
OP(optimisationReturnObjectST)

OP(optimisationValueArray32Next)	//for float/int
OP(optimisationValueArray64Next)	//for point
OP(optimisationObjectArrayNextST)
