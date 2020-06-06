function array_is_equal(first_array, second_array){
    if (first_array.length == second_array.length){
        for(let i = 0; i < first_array.length; i++){
            if (second_array.indexOf(first_array[i]) == -1) return false;
        }
    }
    else return false;

    return true;
}