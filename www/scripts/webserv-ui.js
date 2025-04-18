// Webserv Modern UI - Script d'interface

// Fonction pour créer des particules dans les boîtes
function createParticles() {
  const boxes = document.querySelectorAll('.test-box');
  
  boxes.forEach(box => {
    // Créer l'élément qui contiendra les particules
    const particles = document.createElement('div');
    particles.className = 'particles';
    
    // Nombre aléatoire de particules entre 10 et 15
    const particleCount = Math.floor(Math.random() * 6) + 10;
    
    // Créer les particules
    for (let i = 0; i < particleCount; i++) {
      const particle = document.createElement('span');
      
      // Position aléatoire
      const posX = Math.random() * 100;
      const posY = Math.random() * 100;
      
      // Taille aléatoire
      const size = Math.random() * 5 + 2;
      
      // Délai d'animation aléatoire
      const delay = Math.random() * 2;
      
      // Style des particules
      particle.style.left = `${posX}%`;
      particle.style.top = `${posY}%`;
      particle.style.width = `${size}px`;
      particle.style.height = `${size}px`;
      particle.style.animationDelay = `${delay}s`;
      particle.style.animationDuration = `${Math.random() * 4 + 3}s`;
      
      particles.appendChild(particle);
    }
    
    // Ajouter les particules à la boîte
    box.appendChild(particles);
  });
}

// Ajouter un effet de hover amélioré pour les boîtes
function enhanceBoxInteraction() {
  const boxes = document.querySelectorAll('.test-box');
  
  boxes.forEach(box => {
    box.addEventListener('mouseenter', () => {
      // Ajouter un effet de mise en évidence
      box.style.transform = 'translateY(-5px)';
      box.style.boxShadow = '0 15px 30px rgba(0, 0, 0, 0.3)';
    });
    
    box.addEventListener('mouseleave', () => {
      // Revenir à l'état normal
      box.style.transform = '';
      box.style.boxShadow = '';
    });
  });
}

// Créer une popup de confirmation
function createPopup(title, message, type = 'success') {
  // Supprimer toute popup existante
  const existingPopup = document.querySelector('.result-popup');
  if (existingPopup) {
    existingPopup.remove();
  }

  const popup = document.createElement('div');
  popup.className = `result-popup ${type}`;
  
  // Ajouter une animation d'entrée
  popup.style.animation = 'popupSlideIn 0.3s ease-out forwards';
  
  // Contenu de la popup
  popup.innerHTML = `
    <div class="popup-icon">${type === 'success' ? '✅' : '❌'}</div>
    <div class="popup-content">
      <h3>${title}</h3>
      <p>${message}</p>
    </div>
    <button class="popup-close">×</button>
  `;
  
  // Ajouter la popup au DOM
  document.body.appendChild(popup);
  
  // Ajouter l'événement de fermeture
  const closeBtn = popup.querySelector('.popup-close');
  closeBtn.addEventListener('click', () => {
    popup.style.animation = 'popupSlideOut 0.3s ease-in forwards';
    setTimeout(() => {
      popup.remove();
    }, 300);
  });
  
  // Fermeture automatique après 5 secondes
  setTimeout(() => {
    if (document.body.contains(popup)) {
      popup.style.animation = 'popupSlideOut 0.3s ease-in forwards';
      setTimeout(() => {
        if (document.body.contains(popup)) {
          popup.remove();
        }
      }, 300);
    }
  }, 5000);

  return popup;
}

// Gestion des résultats de test
function setupTestResults() {
  // Ajouter les styles pour les popups
  if (!document.getElementById('popup-styles')) {
    const style = document.createElement('style');
    style.id = 'popup-styles';
    style.textContent = `
      .result-popup {
        position: fixed;
        bottom: 30px;
        right: 30px;
        background-color: var(--box-bg);
        border-radius: 12px;
        padding: 20px;
        box-shadow: 0 10px 30px rgba(0, 0, 0, 0.3);
        display: flex;
        align-items: center;
        min-width: 300px;
        max-width: 500px;
        z-index: 1000;
        overflow: hidden;
      }
      
      .result-popup.success {
        border-left: 5px solid var(--accent-color);
      }
      
      .result-popup.error {
        border-left: 5px solid var(--delete-color);
      }
      
      .popup-icon {
        font-size: 24px;
        margin-right: 15px;
      }
      
      .popup-content {
        flex-grow: 1;
      }
      
      .popup-content h3 {
        margin: 0 0 5px 0;
        color: var(--primary-text);
      }
      
      .popup-content p {
        margin: 0;
        color: var(--secondary-text);
      }
      
      .popup-close {
        background: none;
        border: none;
        color: var(--secondary-text);
        font-size: 24px;
        cursor: pointer;
        padding: 0;
        margin-left: 10px;
        line-height: 1;
      }
      
      .popup-close:hover {
        color: var(--primary-text);
      }
      
      @keyframes popupSlideIn {
        from {
          transform: translateX(100%);
          opacity: 0;
        }
        to {
          transform: translateX(0);
          opacity: 1;
        }
      }
      
      @keyframes popupSlideOut {
        from {
          transform: translateX(0);
          opacity: 1;
        }
        to {
          transform: translateX(100%);
          opacity: 0;
        }
      }
    `;
    document.head.appendChild(style);
  }

  // Initialiser la div de résultat si elle n'existe pas déjà
  if (!document.getElementById('test-result')) {
    const resultDiv = document.createElement('div');
    resultDiv.id = 'test-result';
    resultDiv.className = 'result';
    resultDiv.style.display = 'none';
    document.querySelector('.container').appendChild(resultDiv);
  }
}

// Fonction pour afficher les résultats de test
function displayTestResult(message, isSuccess = true) {
  const resultDiv = document.getElementById('test-result');
  resultDiv.style.display = 'block';
  resultDiv.textContent = message;
  resultDiv.className = `result ${isSuccess ? 'success' : 'error'}`;
  
  // Faire défiler jusqu'au résultat
  resultDiv.scrollIntoView({ behavior: 'smooth', block: 'center' });
}

// Fonction d'initialisation
function initUI() {
  createParticles();
  enhanceBoxInteraction();
  setupTestResults();
  
  // Ajouter une classe au body une fois chargé pour les animations d'entrée
  document.body.classList.add('loaded');
}

// Initialiser l'interface quand le DOM est chargé
document.addEventListener('DOMContentLoaded', initUI);

// Fonctions pour les tests HTTP
async function testMethod(path, method = 'GET', description) {
  // Créer une popup avec un message de chargement
  const popup = createPopup('Test en cours', `Test ${method} en cours : ${description}...`, 'success');
  
  try {
    const startTime = performance.now();
    const options = { method };
    
    const response = await fetch(path, options);
    const endTime = performance.now();
    const duration = (endTime - startTime).toFixed(2);
    
    const contentType = response.headers.get('Content-Type');
    const status = response.status;
    
    let result = `📡 Test ${method} : ${path}\n`;
    result += `⏱️ Temps de réponse : ${duration}ms\n`;
    result += `📥 Status : ${status} ${response.statusText}\n`;
    result += `📎 Type : ${contentType || 'non spécifié'}\n`;
    
    // Mettre à jour la popup avec le résultat
    if (response.ok) {
      popup.innerHTML = `
        <div class="popup-icon">✅</div>
        <div class="popup-content">
          <h3>Test ${method} réussi</h3>
          <p>Status: ${status}, Temps: ${duration}ms</p>
        </div>
        <button class="popup-close">×</button>
      `;
      
      // Attacher à nouveau l'événement au bouton de fermeture
      const closeBtn = popup.querySelector('.popup-close');
      closeBtn.addEventListener('click', () => {
        popup.style.animation = 'popupSlideOut 0.3s ease-in forwards';
        setTimeout(() => {
          popup.remove();
        }, 300);
      });
      
      // Si le test est réussi, on peut rediriger vers la page cible après un court délai
      setTimeout(() => {
        window.location.href = path;
      }, 1000);
    } else {
      popup.innerHTML = `
        <div class="popup-icon">❌</div>
        <div class="popup-content">
          <h3>Test ${method} échoué</h3>
          <p>Status: ${status}, Erreur: ${response.statusText}</p>
        </div>
        <button class="popup-close">×</button>
      `;
      popup.className = 'result-popup error';
      
      // Attacher à nouveau l'événement au bouton de fermeture
      const closeBtn = popup.querySelector('.popup-close');
      closeBtn.addEventListener('click', () => {
        popup.style.animation = 'popupSlideOut 0.3s ease-in forwards';
        setTimeout(() => {
          popup.remove();
        }, 300);
      });
    }
    
  } catch (error) {
    // Mettre à jour la popup avec l'erreur
    popup.innerHTML = `
      <div class="popup-icon">❌</div>
      <div class="popup-content">
        <h3>Erreur lors du test ${method}</h3>
        <p>${error.message}</p>
      </div>
      <button class="popup-close">×</button>
    `;
    popup.className = 'result-popup error';
    
    // Attacher à nouveau l'événement au bouton de fermeture
    const closeBtn = popup.querySelector('.popup-close');
    closeBtn.addEventListener('click', () => {
      popup.style.animation = 'popupSlideOut 0.3s ease-in forwards';
      setTimeout(() => {
        popup.remove();
      }, 300);
    });
  }
  
  return false; // Pour empêcher la navigation par défaut
}

// Tests pour chaque méthode et fonctionnalité
function testGet() {
  window.location.href = '/GET.html';
  return false;
}

function testPost() {
  window.location.href = '/POST.html';
  return false;
}

function testDelete() {
  window.location.href = '/delete.html';
  return false;
}

function testUpload() {
  window.location.href = '/pages/file-upload.html';
  return false;
}

function testDownload() {
  window.location.href = '/download/';
  return false;
}

function testFileTransfer() {
  window.location.href = '/filetransfert/';
  return false;
}

function testListDirectory() {
  window.location.href = '/list/';
  return false;
}

function testRedirection() {
  createPopup('Redirection', 'Vous allez être redirigé vers youtube.com dans 3 secondes...', 'success');
  
  setTimeout(() => {
    window.location.href = '/youtube.com';
  }, 3000);
  
  return false;
}

function testErrorPage() {
  window.location.href = '/non-existent-page';
  return false;
}

function testAlias() {
  window.location.href = '/banana.html';
  return false;
}

// Export des fonctions pour utilisation globale
window.testMethod = testMethod;
window.displayTestResult = displayTestResult;
window.testGet = testGet;
window.testPost = testPost;
window.testDelete = testDelete;
window.testUpload = testUpload;
window.testDownload = testDownload;
window.testFileTransfer = testFileTransfer;
window.testListDirectory = testListDirectory;
window.testRedirection = testRedirection;
window.testErrorPage = testErrorPage;
window.testAlias = testAlias; 