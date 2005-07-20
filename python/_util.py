import re
from exceptions import ValidationException

def _validate_interface_or_name(value):
    elements = value.split('.')
    if len(elements) <= 1:
        raise ValidationException("%s must contain at least two elements seperated by a period ('.')"%(value))

    validate = re.compile('[A-Za-z][\w_]*')
    for element in elements:
        if not validate.match(element):
            raise ValidationException("Element %s of %s has invalid characters"%(element ,value))

