"""Лапки любви: состояние истории и обработчики событий браузера."""
from random import choice, randint

SCENES = ("start", "intro", "pet", "flowers", "letters", "gift", "final")
FLOWERS = {
    "rose": ("🌹", "Пусть тебя всегда окружает любовь!"),
    "tulip": ("🌷", "Желаю тебе крепкого здоровья!"),
    "daisy": ("🌼", "Пусть каждый день приносит радость!"),
    "peony": ("🌸", "Пусть сбываются самые заветные желания!"),
    "sunflower": ("🌻", "Пусть в твоей жизни всегда будет тепло и светло!"),
}
LETTERS = {
    "wish": "Мамочка, пусть у тебя будет больше спокойных, светлых дней, приятных неожиданностей и времени для себя. Ты заслуживаешь всего самого доброго!",
    "hug": "Лови самые крепкие объятия! Даже когда мы не рядом, я всегда думаю о тебе. А пёсик передаёт ещё одну пушистую обнимашку. ♡",
    "son": "Мама, спасибо тебе за всё, чему ты меня научила, за заботу и за то, что всегда рядом. Я не всегда говорю это вслух, но я очень тебя люблю. Твой сын. ♡",
}
PET_MESSAGES = ("Я уже виляю хвостиком!", "Ой, как приятно!", "Ещё немножко, пожалуйста!", "Ты самая добрая!", "Почти полное сердечко!", "Ура! Теперь я наполнен любовью до самых ушек!")


class Game:
    """Независимая от браузера логика. Тестируется обычным Python."""
    def __init__(self):
        self.reset()

    def reset(self):
        self.scene = "start"
        self.love = 0
        self.flowers = []
        self.letters = set()
        self.gift_opened = False

    def advance(self):
        """Переходит вперёд только при выполненном условии сцены."""
        ready = {"start": True, "intro": True, "pet": self.love == 5,
                 "flowers": len(self.flowers) == len(FLOWERS),
                 "letters": len(self.letters) == len(LETTERS),
                 "gift": self.gift_opened, "final": False}
        if not ready[self.scene]:
            return False
        self.scene = SCENES[SCENES.index(self.scene) + 1]
        return True

    def pet(self):
        if self.scene != "pet" or self.love >= 5:
            return False
        self.love += 1
        return True

    def collect(self, key):
        if self.scene != "flowers" or key not in FLOWERS or key in self.flowers:
            return False
        self.flowers.append(key)
        return True

    def open_letter(self, key):
        if self.scene != "letters" or key not in LETTERS:
            return False
        self.letters.add(key)
        return True

    def open_gift(self):
        if self.scene != "gift" or self.gift_opened:
            return False
        self.gift_opened = True
        return True


def mount(document, when):
    """Связывает Python с HTML. Все игровые действия работают здесь."""
    game = Game()

    def el(id):
        return document.getElementById(id)

    def text(id, value):
        el(id).textContent = value

    def burst(symbols="♡♥", amount=16):
        """Создаёт ограниченное количество анимированных частиц."""
        layer = el("particles")
        layer.textContent = ""
        for _ in range(amount):
            particle = document.createElement("span")
            particle.className = "particle"
            particle.textContent = choice(symbols)
            for name, value in (("--x", f"{randint(8,92)}%"),
                                ("--size", f"{randint(15,35)}px"),
                                ("--delay", f"{randint(0,350)}ms"),
                                ("--dx", f"{randint(-85,85)}px"),
                                ("--rot", f"{randint(-55,55)}deg"),
                                ("--color", choice(("#bd4268", "#e48e9e", "#ddab58", "#a78ab7")))):
                particle.style.setProperty(name, value)
            layer.appendChild(particle)

    def show_scene(focus=True):
        """Показывает ровно один экран и обновляет общий прогресс."""
        for name in SCENES:
            el(name).hidden = name != game.scene
        n = SCENES.index(game.scene)
        text("chapter", f"Глава {n + 1} из {len(SCENES)}")
        dots = el("dots").children
        for i in range(len(SCENES)):
            dots.item(i).className = "done" if i < n else "active" if i == n else ""
        el("particles").textContent = ""
        if focus:
            el(game.scene + "-title").focus()

    @when("click", "#start-button, #intro-next, #pet-next, #flowers-next, #letters-next, #gift-next")
    def next_scene(event):
        if game.advance():
            show_scene()
            if game.scene == "final":
                burst("♡♥✦", 32)

    def update_pet():
        text("hearts", " ".join("♥" if i < game.love else "♡" for i in range(5)))
        el("love-progress").value = game.love
        text("love-status", f"Уровень любви: {game.love} из 5")
        text("pet-message", PET_MESSAGES[game.love])
        el("pet-button").hidden = game.love == 5
        el("pet-next").hidden = game.love != 5

    @when("click", "#pet-picture, #pet-button")
    def pet_dog(event):
        if game.scene != "pet":
            return
        changed = game.pet()
        if changed:
            update_pet()
        picture = el("pet-picture")
        picture.classList.remove("pet-animate")
        # Чтение layout перезапускает CSS-анимацию при быстрых кликах.
        _ = picture.offsetWidth
        picture.classList.add("pet-animate")
        burst(amount=10)

    @when("click", ".flower")
    def collect_flower(event):
        key = str(event.currentTarget.getAttribute("data-flower"))
        if not game.collect(key):
            return
        el("flower-" + key).disabled = True
        text("flower-message", FLOWERS[key][1])
        text("flower-status", f"Собрано {len(game.flowers)} из {len(FLOWERS)}")
        text("bouquet", " ".join(FLOWERS[k][0] for k in game.flowers))
        el("flowers-next").disabled = len(game.flowers) != len(FLOWERS)
        burst("✿♡", 8)

    @when("click", ".letter")
    def open_letter(event):
        key = str(event.currentTarget.getAttribute("data-letter"))
        if not game.open_letter(key):
            return
        for name in LETTERS:
            button = el("letter-" + name)
            button.classList.toggle("opened", name in game.letters)
            button.classList.toggle("selected", name == key)
            button.setAttribute("aria-pressed", "true" if name == key else "false")
        text("letter-message", LETTERS[key])
        text("letter-status", f"Открыто {len(game.letters)} из {len(LETTERS)}")
        el("letters-next").disabled = len(game.letters) != len(LETTERS)
        burst("♡", 7)

    @when("click", "#gift-box, #gift-button")
    def open_gift(event):
        if not game.open_gift():
            return
        el("gift-box").disabled = True
        el("gift-box").classList.add("opening")
        el("gift-button").hidden = True
        el("gift").querySelector(".art img").src = "assets/dog/gift.webp"
        text("gift-message", "Та-дам! Самый главный подарок — моя любовь к тебе!")
        el("gift-next").hidden = False
        burst("♡✦✿●", 36)

    @when("animationend", "#gift-box")
    def finish_gift(event):
        if game.gift_opened:
            el("gift-box").classList.add("opened")

    @when("click", "#hug-button")
    def hug(event):
        if game.scene == "final":
            text("hug-message", "Обнимаю крепко-крепко! ♡")
            burst("♡♥", 28)

    @when("click", "#restart-button")
    def restart(event):
        game.reset()
        update_pet()
        el("pet-picture").classList.remove("pet-animate")
        for key in FLOWERS:
            el("flower-" + key).disabled = False
        text("flower-status", "Собрано 0 из 5")
        text("flower-message", "Выбери первый цветочек.")
        text("bouquet", "")
        el("flowers-next").disabled = True
        for key in LETTERS:
            button = el("letter-" + key)
            button.classList.remove("opened", "selected")
            button.setAttribute("aria-pressed", "false")
        text("letter-message", "Какое письмо откроем первым?")
        text("letter-status", "Открыто 0 из 3")
        el("letters-next").disabled = True
        el("gift-box").classList.remove("opening", "opened")
        el("gift-box").disabled = False
        el("gift").querySelector(".art img").src = "assets/dog/intro.webp"
        el("gift-button").hidden = False
        el("gift-next").hidden = True
        text("gift-message", "Интересно, что же внутри?")
        text("hug-message", "")
        show_scene()

    show_scene(focus=False)
    el("start-button").disabled = False
    text("loading", "Нажимай, когда будешь готова ♡")
    return game


# В обычном Python импортируется только логика, а в браузере — и интерфейс.
try:
    from pyscript import document, when
except ImportError:
    pass
else:
    mount(document, when)
