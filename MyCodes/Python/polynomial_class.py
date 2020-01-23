#!/usr/bin/env python3

#		Michal Glos
#		Polynomial class
#
#		The objective of this project is to create polynomial
#		class, with python "magic" methods equal, not equal
#		adding, power and string representation. Implementation
#		should also include its derivative, its value when x is constant
#		and differention of function value between two parameters (Function at_value)
#

class Polynomial():
    """ Class which can hold polynoms and perform basic operations over them such as addition, exponentioation, derivation, differention and comparison """

    def __init__(self, *args, **kwargs):
        """ Initializes new instance of Polynomial class, accepts several ints as args, list of ints or keyword arguments"""
        pol_list = list()                                           # list for adding *args
        for arg in args:                                            # iterate through args
            if type(arg) == list:
                for i in arg[:1:-1]:                                # getting rid of redundant zeros at the end of the list
                    if i == 0:
                        arg.pop()
                    else:
                        break
                self.polynom_list = arg                             # if arg is list, save list as polynom_list and return
                return None
            else:                                                   # else append the value
                pol_list.append(arg)
        for variable, value in kwargs.items():                      # iterate through **kwargs
            while int(variable[1:]) >= len(pol_list):               # of index out of range, append zeros to polynom_list, then rewrite them with actual value
                pol_list.append(0)
            pol_list[int(variable[1:])] = value
        for i in pol_list[:1:-1]:                                   # getting rid of redundant zeros at the end of the list
            if i == 0:
                pol_list.pop()
            else:
                break
        self.polynom_list = pol_list                                

    def __eq__(self, other):
        """ self == other - returns true if objects are instances of Polynomial and their polynoms are equal"""
        if not isinstance(other, Polynomial):                                                           # can compare just instances of the same class
            return NotImplemented
        cmp_list_self, cmp_list_other = self.polynom_list.copy(), other.polynom_list.copy()             # copying list of polynom values from both operands
        for i in cmp_list_self[::-1]:                                                                   # getting rid of redundant zeros at the end of the list
            if i == 0:
                cmp_list_self.pop()
            else:
                break
        for i in cmp_list_other[::-1]:                                                                  # getting rid of redundant zeros at the end of the list
            if i == 0:
                cmp_list_other.pop()
            else:
                break
        return cmp_list_self == cmp_list_other                                                          # copmaring both lists

    def __ne__(self, other):
        """ logical not of equal function """
        return not self.__eq__(other)                                           # returns negated __eq__ function

    def __add__(self, other):
        """ Allows user to add two Polynomial classes, returns sum of polynoms"""
        if not isinstance(other, Polynomial):                                           # if different class instance, return NotImplemented
            return NotImplemented
        long_list, short_list = (self.polynom_list.copy(), other.polynom_list.copy()) if len(self.polynom_list) >= len(other.polynom_list) else (other.polynom_list.copy(), self.polynom_list.copy())
        # Assigning longer of operands to longer list and vice versa
        for i in range(len(short_list)):                                                # iterates through longer list, if there is a value in shorter list on the same index, sums both values and saves them to longer list
            if i < len(short_list):
                long_list[i] += short_list[i]
            else:
                return Polynomial(long_list)                                            # if iterated to the end of short list, next values of longer list won't be changed, returns longer list
        return Polynomial(long_list)

    def __pow__(self, other):
        """ Allows pow of Polynomial instance by other (int), returns Polynomial """
        if not isinstance(other, int):                                                          # if exponent is not int, function is not defined
            return NotImplemented                                      
        powered_polynom_list = [0] * len(self.polynom_list)                                     # empty polynom, numbers to be assigned later
        old_polynom_list = self.polynom_list.copy()                                             # copy of Polynomial instance (for counting)
        for i in range(other - 1):                                                              # repeating polynom multiplication
            powered_polynom_list += [0] * ((len(self.polynom_list.copy())) - 1)                 # add to powered polygon space for larger exponents (double neght - 1)
            for j in range(len(powered_polynom_list) - (len(self.polynom_list)) + 1):           # iterate through powered polynom, multiply each item
                for k in range(len(self.polynom_list)):                                         # multiply each element
                    powered_polynom_list[j + k] += old_polynom_list[j] * self.polynom_list[k]   # multiplying exponnents, add them => x^j * x^k = x^(j+k)
            old_polynom_list = powered_polynom_list.copy()                                      # moving result to old list for nex iteration
            powered_polynom_list = [0] * len(powered_polynom_list)                              # assign zeros to blank list for next iteration
        return Polynomial(old_polynom_list)

    def __str__(self):
        """ Prints polynom_list. Returns polynom as string (including multiplied X) """
        if set(self.polynom_list)== {0}:                                                            # if list contains just zeros, return "0"
            return "0"
        polynom = ""
        for i in range(0, len(self.polynom_list)):                                                  # iterate through polynom to determine its format of variable
            if(self.polynom_list[i] == 0):                                                          # if 0x^n => ""
                continue
            elif( i == 0 ):                                                                         # x^0 = 1 => 3x^0 = 3
                polynom = str(self.polynom_list[i])
            elif( abs(self.polynom_list[i]) == 1 ):                                                 # 1x^n = x^n
                polynom = ("" if self.polynom_list[i] > 0 else "-") + "x^{} + ".format(i) + polynom if polynom != "" else "x^{}".format(i)
            else:                                                                                   # full form of polygon nx^p
                polynom = "{}x^{} + ".format(str(self.polynom_list[i]), i) + polynom if polynom != "" else "{}x^{}".format(str(self.polynom_list[i]), i)
        return polynom.replace(" + -", " - ").replace("x^1", "x")

    def derivative(self):
        """ Returns derivated polynom as instance of Polynomial class"""
        if len(self.polynom_list) == 1:                                             # if polynom contains just constnt, it's derivative is 0
            return Polynomial(0)
        derivatited_list = [0] * (len(self.polynom_list) - 1)                       # blank list to assign derivated polynom
        for i in range(1, len(self.polynom_list)):                                  # each expression is derivated and assigned to derivated list
            derivatited_list[i - 1] = self.polynom_list[i] * i
        return Polynomial(derivatited_list)
        
    def at_value(self, *args):
        """ Calculates difference of function values of 2 arguments ( f(arg2) - f(arg2) ), if only one arguemnt is passed, returns f(arg) """
        polynom_sum = 0
        for number in args:                                                     # iterates through arguments
            polynom_sum *= -1                                                   # in case of second iteration (2 args) first value is negated
            if type(number) == int or float:                                    # check of argument datatypes
                for i in range(len(self.polynom_list)):                         # sum all polynom items
                    polynom_sum += self.polynom_list[i] * (number**i)    
            else:
                return NotImplemented
        return polynom_sum
