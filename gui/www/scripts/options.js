function updateOptionBalanceness() {
    let balancenessOption = document.getElementById('smts-option-balanceness');
    let balancenessHalos = document.querySelectorAll('.smts-balanceness');
    if (balancenessOption.checked) {
        for (let balancenessHalo of balancenessHalos) {
            balancenessHalo.classList.remove('smts-hidden');
        }
    }
    else {
        for (let balancenessHalo of balancenessHalos) {
            balancenessHalo.classList.add('smts-hidden');
        }
    }
}